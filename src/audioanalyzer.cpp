// SPDX-License-Identifier: GPL-3.0-or-later
#include "audioanalyzer.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace {
bool stringEnd(const QByteArray &data, qsizetype start, qsizetype &end) {
    for(qsizetype i=start+1;i<data.size();++i) {
        if(data[i]=='\\') {
            if(++i>=data.size())return false;
        } else if(data[i]=='"') {
            end=i;return true;
        }
    }
    return false;
}

bool containerEnd(const QByteArray &data, qsizetype start, qsizetype &end) {
    std::vector<char> containers;
    for(qsizetype i=start;i<data.size();++i) {
        const char character=data[i];
        if(character=='"') {
            if(!stringEnd(data,i,i))return false;
        } else if(character=='{' || character=='[')containers.push_back(character);
        else if(character=='}' || character==']') {
            if(containers.empty() || containers.back()!=(character=='}'?'{':'['))return false;
            containers.pop_back();
            if(containers.empty()){end=i;return true;}
        }
    }
    return false;
}

qsizetype skipSpace(const QByteArray &data, qsizetype position) {
    while(position<data.size()) {
        const char character=data[position];
        if(character!=' ' && character!='\t' && character!='\n' && character!='\r')break;
        ++position;
    }
    return position;
}

QByteArray withoutPipeWireParams(const QByteArray &dump, bool &valid) {
    // Older pw-dump versions can serialize unknown newer SPA POD parameters as
    // invalid JSON (bare arrays inside an object). We only consume properties
    // and metadata. Drop complete params objects before JSON parsing, without
    // confusing braces or the word "params" inside quoted/escaped strings.
    QByteArray clean;
    clean.reserve(dump.size());
    qsizetype copied=0;
    valid=false;
    for(qsizetype i=0;i<dump.size();++i) {
        if(dump[i]!='"')continue;
        qsizetype end=0;
        if(!stringEnd(dump,i,end))return {};
        if(end-i==7 && dump.mid(i,8)=="\"params\"") {
            qsizetype value=skipSpace(dump,end+1);
            if(value<dump.size() && dump[value]==':') {
                value=skipSpace(dump,value+1);
                if(value<dump.size() && dump[value]=='{') {
                    qsizetype close=0;
                    if(!containerEnd(dump,value,close))return {};
                    clean.append(dump.constData()+copied,value-copied);
                    clean.append("{}");
                    copied=close+1;i=close;continue;
                }
            }
        }
        i=end;
    }
    clean.append(dump.constData()+copied,dump.size()-copied);
    valid=true;
    return clean;
}
}

namespace Phosrack {
AudioOutputs parseAudioOutputs(const QByteArray &dump) {
    AudioOutputs outputs;
    bool balanced=false;
    const auto clean=withoutPipeWireParams(dump,balanced);
    if(!balanced)return outputs;
    const auto objects=QJsonDocument::fromJson(clean).array();
    QString defaultName;
    for(const auto &entry:objects) {
        const auto object=entry.toObject();
        if(object.value("type").toString()!="PipeWire:Interface:Metadata" ||
           object.value("props").toObject().value("metadata.name").toString()!="default")continue;
        for(const auto &value:object.value("metadata").toArray()) {
            const auto item=value.toObject();
            if(item.value("subject").toInt(-1)!=0 || item.value("key").toString()!="default.audio.sink")continue;
            auto target=item.value("value").toObject();
            if(item.value("value").isString())target=QJsonDocument::fromJson(item.value("value").toString().toUtf8()).object();
            defaultName=target.value("name").toString();
        }
    }
    for(const auto &entry:objects) {
        const auto props=entry.toObject().value("info").toObject().value("props").toObject();
        // Even if metadata points at an input, only output sinks are eligible.
        if(props.value("media.class").toString()!="Audio/Sink")continue;
        const auto serial=props.value("object.serial").toVariant().toString();
        if(!QRegularExpression("^[0-9]+$").match(serial).hasMatch())continue;
        const auto name=props.value("node.name").toString();
        if(name.isEmpty())continue;
        outputs.sources<<QVariantMap{{"id",name},{"serial",serial},{"objectId",entry.toObject().value("id").toInt(-1)},
            {"name",props.value("node.description").toString(name)}};
        if(!defaultName.isEmpty() && name==defaultName)outputs.defaultId=name;
    }
    return outputs;
}
}

AudioAnalyzer::AudioAnalyzer(QObject *parent):QObject(parent),m_status(tr("Visualizer paused")) {
    for(int i=0;i<Phosrack::BandCount;++i)m_bands<<0.;
    for(int i=0;i<128;++i)m_wave<<0.;
    m_frames.setInterval(33);
    connect(&m_frames,&QTimer::timeout,this,&AudioAnalyzer::updateFrame);
    connect(&m_capture,&QProcess::readyReadStandardOutput,this,&AudioAnalyzer::consume);
    connect(&m_capture,&QProcess::readyReadStandardError,this,[this]{m_capture.readAllStandardError();});
    connect(&m_capture,&QProcess::started,this,[this]{
        if(!m_active){stopCapture();return;}
        setStatus(tr("Listening to system output"));m_frames.start();
    });
    connect(&m_capture,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){
        if(m_requested){m_requested=false;m_frames.stop();
            setStatus(tr("Audio capture unavailable. Check PipeWire and pw-cat."));}
    });
    connect(&m_capture,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int,QProcess::ExitStatus){
        m_frames.stop();m_level=0;m_history={};m_bands.fill(0.);m_wave.fill(0.);resetPitch();resetMeter();emit frameReady();
        if(m_requested){m_requested=false;setStatus(tr("Audio output disconnected"));}
        else emit changed();
    });
    m_discoverTimeout.setSingleShot(true);
    connect(&m_discoverTimeout,&QTimer::timeout,this,[this]{
        m_discover.kill();setStatus(tr("PipeWire did not respond"));
    });
    connect(&m_discover,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){
        m_discoverTimeout.stop();setStatus(tr("PipeWire tools are unavailable"));
    });
    connect(&m_discover,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int exitCode,QProcess::ExitStatus){
        m_discoverTimeout.stop();m_discover.readAllStandardError();
        if(exitCode!=0){setStatus(tr("Cannot read system audio outputs"));return;}
        const auto outputs=Phosrack::parseAudioOutputs(m_discover.readAllStandardOutput());
        const bool listChanged=m_sources!=outputs.sources;
        m_sources=outputs.sources;m_defaultSource=outputs.defaultId;
        if(listChanged)emit sourcesChanged();
        resolveOutput();
    });
    m_routePoll.setInterval(2000);
    connect(&m_routePoll,&QTimer::timeout,this,&AudioAnalyzer::refreshSources);
}

AudioAnalyzer::~AudioAnalyzer() {
    m_active=false;m_routePoll.stop();stopCapture();
    if(m_discover.state()!=QProcess::NotRunning){m_discover.kill();m_discover.waitForFinished(500);}
}

void AudioAnalyzer::setStatus(const QString &value) {
    if(m_status!=value){m_status=value;emit changed();}
}

void AudioAnalyzer::setActive(bool value) {
    if(value==m_active)return;
    m_active=value;emit changed();
    if(!m_active){
        m_routePoll.stop();stopCapture();setStatus(tr("Visualizer paused"));return;
    }
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")){
        setStatus(tr("Waiting for audio"));return;
    }
    m_routePoll.start();refreshSources();
}

void AudioAnalyzer::setSource(const QString &id) {
    if(id==m_source)return;
    if(!id.isEmpty()&&!QRegularExpression("^[a-zA-Z0-9_.:-]+$").match(id).hasMatch())return;
    m_source=id;emit sourcesChanged();resolveOutput();
}

void AudioAnalyzer::refreshSources() {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    if(m_discover.state()!=QProcess::NotRunning)return;
    m_discover.start("pw-dump",{});m_discoverTimeout.start(5000);
}

void AudioAnalyzer::resolveOutput() {
    const auto requested=m_source.isEmpty()?m_defaultSource:m_source;
    QString target,name;
    for(const auto &entry:m_sources) {
        const auto output=entry.toMap();
        if(output.value("id").toString()==requested){target=output.value("serial").toString();name=output.value("name").toString();break;}
    }
    if(target!=m_resolvedSource){
        stopCapture();m_resolvedSource=target;m_outputName=name;emit sourcesChanged();
    } else if(m_outputName!=name){m_outputName=name;emit sourcesChanged();}
    if(!m_active)return;
    if(target.isEmpty()){
        setStatus(m_source.isEmpty()?tr("No default audio output available"):tr("Selected audio output unavailable"));return;
    }
    startCapture();
}

void AudioAnalyzer::start(){setActive(true);}
void AudioAnalyzer::stop(){setActive(false);}

void AudioAnalyzer::startCapture() {
    if(!m_active || qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE") || m_resolvedSource.isEmpty() ||
       m_capture.state()!=QProcess::NotRunning)return;
    const auto binary=QStandardPaths::findExecutable("pw-cat");
    if(binary.isEmpty()){setStatus(tr("PipeWire tools are unavailable"));return;}
    m_history={};m_pending.clear();m_requested=true;
    // Pin monitor ports to the resolved sink. Discovery follows route changes;
    // PipeWire must never reconnect this stream to an input or an arbitrary node.
    m_capture.start(binary,{"--record","--raw","--format=f32","--rate=48000","--channels=2","--latency=50ms",
        "--target",m_resolvedSource,"--properties","{ stream.capture.sink = true node.passive = true node.dont-reconnect = true }","-"});
    setStatus(tr("Opening audio output…"));
}

void AudioAnalyzer::stopCapture() {
    m_requested=false;m_frames.stop();
    if(m_capture.state()!=QProcess::NotRunning){
        m_capture.terminate();if(!m_capture.waitForFinished(400)){m_capture.kill();m_capture.waitForFinished(400);}
    }
    m_pending.clear();m_history={};m_bands.fill(0.);m_wave.fill(0.);m_level=0;resetPitch();resetMeter();
    emit changed();emit frameReady();
}

void AudioAnalyzer::consume() {
    m_pending+=m_capture.readAllStandardOutput();
    const int complete=int(m_pending.size()/8)*8;
    // Retain recent complete stereo frames if the UI was suspended.
    const int begin=std::max(0,complete-Phosrack::PitchFftSize*8);
    for(int i=begin;i<complete;i+=8){
        float l=0,r=0;std::memcpy(&l,m_pending.constData()+i,4);std::memcpy(&r,m_pending.constData()+i+4,4);
        l=std::isfinite(l)?std::clamp(l,-1.f,1.f):0.f;
        r=std::isfinite(r)?std::clamp(r,-1.f,1.f):0.f;
        m_history.append((l+r)*.5f,l,r);
    }
    m_pending.remove(0,complete);
}

bool AudioAnalyzer::readAudioFrame(std::array<float, Phosrack::FftSize> &samples, Phosrack::PitchSamples *pitch,
                                   Phosrack::StereoMeterFrame &meter) {
    meter = m_history.takeMeterFrame();
    if (!m_history.takeFresh()) return false;
    m_history.copyRecent(samples);
    if (pitch) m_history.copyRecent(*pitch);
    return true;
}
