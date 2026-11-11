// SPDX-License-Identifier: GPL-3.0-or-later
#include "equalizer.h"
#include "audioanalyzer.h"
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>
#include <array>
#include <cmath>

namespace {
struct Preset { const char *id; std::array<double,10> gains; };
constexpr std::array<Preset,11> curves{{
    {"flat", {0,0,0,0,0,0,0,0,0,0}},
    {"rock", {4,3,1,-1,-2,-1,1,3,4,3}},
    {"pop", {-1,1,3,4,3,0,-1,-1,0,1}},
    {"jazz", {3,2,0,1,-1,-1,0,1,3,4}},
    {"classical", {3,2,1,0,-2,-2,0,1,2,3}},
    {"bass", {8,6,4,2,0,0,0,0,0,0}},
    {"vocal", {-3,-2,-1,0,2,4,4,2,0,-2}},
    {"electronic", {5,4,1,-2,-2,0,2,3,4,4}},
    {"hiphop", {5,4,1,2,-1,-1,2,0,1,2}},
    {"acoustic", {2,2,1,0,0,1,2,3,2,1}},
    {"treble", {0,0,0,0,0,1,2,4,5,5}}
}};
double bounded(double gain) { return std::isfinite(gain)?std::clamp(gain,-12.0,12.0):0.0; }

}

namespace Phosrack {
QString equalizerControls(const QVariantList &gains,double preamp) {
    QJsonArray values;
    for(int i=0;i<10;++i)values.append(bounded(gains.value(i).toDouble()));
    return QString::fromUtf8(QJsonDocument(QJsonObject{{"gains",values},{"preamp",bounded(preamp)}}).toJson(QJsonDocument::Compact));
}
QString equalizerConfiguration(const QVariantList &gains,double preamp,const QString &output) {
    auto object=QJsonDocument::fromJson(equalizerControls(gains,preamp).toUtf8()).object();
    object["target"]=output;
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}
}

Equalizer::Equalizer(QObject *parent):QObject(parent),m_settings("Phosrack","Phosrack"),
    m_status(tr("Equalizer off")) {
    for(int i=0;i<10;++i)m_gains<<0.0;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")) {
        const auto stored=m_settings.value("eq/gains").toList();
        if(stored.size()==10)for(int i=0;i<10;++i)m_gains[i]=bounded(stored[i].toDouble());
        m_preamp=bounded(m_settings.value("eq/preamp",0.0).toDouble());
        const auto id=m_settings.value("eq/preset","flat").toString();
        if(id=="custom"||std::any_of(curves.begin(),curves.end(),[&](const Preset &p){return id==p.id;}))m_preset=id;
    }
    m_update.setSingleShot(true);m_update.setInterval(80);
    connect(&m_update,&QTimer::timeout,this,&Equalizer::sendControls);
    m_routes.setInterval(2000);connect(&m_routes,&QTimer::timeout,this,&Equalizer::discover);
    m_discoveryTimeout.setSingleShot(true);m_discoveryTimeout.setInterval(5000);
    connect(&m_discoveryTimeout,&QTimer::timeout,this,[this]{m_discovery.kill();if(m_enabled)fail(tr("PipeWire did not respond"));});
    m_startTimeout.setSingleShot(true);m_startTimeout.setInterval(5000);
    connect(&m_startTimeout,&QTimer::timeout,this,[this]{if(m_enabled&&!running())fail(tr("The equalizer output could not be created"));});
    connect(&m_discovery,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus){
        m_discoveryTimeout.stop();
        if(!m_enabled)return;
        if(code!=0){fail(tr("Cannot read system audio outputs"));return;}
        acceptDiscovery();
    });
    connect(&m_discovery,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){
        m_discoveryTimeout.stop();if(m_enabled)fail(tr("PipeWire tools are unavailable"));
    });
    connect(&m_filter,&QProcess::started,this,[this]{m_starting=false;discover();});
    connect(&m_filter,&QProcess::readyReadStandardOutput,this,[this]{
        m_workerOutput+=m_filter.readAllStandardOutput();
        int newline=-1;
        while((newline=int(m_workerOutput.indexOf('\n')))>=0){
            const auto message=QJsonDocument::fromJson(m_workerOutput.left(newline)).object();
            m_workerOutput.remove(0,newline+1);
            const int id=message.value("objectId").toInt();
            if(message.value("ready").toBool()&&id>0){
                m_nodeId=QString::number(id);m_startTimeout.stop();
                m_status=tr("Select Phosrack EQ as the output in your system sound settings");
                emit changed();sendControls();
            }
        }
    });
    connect(&m_filter,&QProcess::readyReadStandardError,this,[this]{m_filter.readAllStandardError();});
    connect(&m_filter,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){
        if(!m_stopping&&m_enabled)fail(tr("Could not start the PipeWire equalizer"));
    });
    connect(&m_filter,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int,QProcess::ExitStatus){
        if(!m_stopping&&m_enabled)fail(tr("The equalizer stopped. Check the selected audio output."));
    });
}
Equalizer::~Equalizer(){stop();}
QVariantList Equalizer::presets() const {
    return {
        QVariantMap{{"id","flat"},{"name",tr("Flat")}},
        QVariantMap{{"id","rock"},{"name",tr("Rock")}},
        QVariantMap{{"id","pop"},{"name",tr("Pop")}},
        QVariantMap{{"id","jazz"},{"name",tr("Jazz")}},
        QVariantMap{{"id","classical"},{"name",tr("Classical")}},
        QVariantMap{{"id","bass"},{"name",tr("Bass boost")}},
        QVariantMap{{"id","vocal"},{"name",tr("Vocal")}},
        QVariantMap{{"id","electronic"},{"name",tr("Electronic")}},
        QVariantMap{{"id","hiphop"},{"name",tr("Hip-hop")}},
        QVariantMap{{"id","acoustic"},{"name",tr("Acoustic")}},
        QVariantMap{{"id","treble"},{"name",tr("Treble boost")}},
        QVariantMap{{"id","custom"},{"name",tr("Custom")}}
    };
}
void Equalizer::save(){
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    m_settings.setValue("eq/gains",m_gains);m_settings.setValue("eq/preamp",m_preamp);m_settings.setValue("eq/preset",m_preset);
}
void Equalizer::setBand(int index,double gain){
    if(index<0||index>=10||!std::isfinite(gain))return;
    gain=bounded(gain);if(m_gains[index].toDouble()==gain)return;
    m_gains[index]=gain;m_preset="custom";m_controlsDirty=true;save();emit gainsChanged();m_update.start();
}
void Equalizer::setPreamp(double gain){
    if(!std::isfinite(gain))return;
    gain=bounded(gain);if(m_preamp==gain)return;
    m_preamp=gain;m_controlsDirty=true;save();emit gainsChanged();m_update.start();
}
void Equalizer::selectPreset(const QString &id){
    if(id=="custom"){m_preset=id;save();emit gainsChanged();return;}
    for(const auto &curve:curves)if(id==curve.id){
        for(int i=0;i<10;++i)m_gains[i]=curve.gains[i];
        m_preset=id;m_controlsDirty=true;save();emit gainsChanged();m_update.start();return;
    }
}
void Equalizer::reset(){selectPreset("flat");}
bool Equalizer::ensureBypassed(){setEnabled(false);return !enabled();}
void Equalizer::setEnabled(bool enabled){
    if(enabled && !m_permitted) return;
    if(enabled==m_enabled)return;
    m_error.clear();
    if(!enabled){stop();m_status=tr("Equalizer off");emit changed();return;}
    setPreamp(0);
    m_enabled=true;m_status=tr("Opening equalizer…");emit changed();
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")){m_status=tr("Equalizer preview — audio unchanged");emit changed();return;}
    if(QStandardPaths::findExecutable("pw-dump").isEmpty()){
        fail(tr("Install PipeWire tools to use the equalizer"));return;
    }
    m_target.clear();m_nodeId.clear();m_routes.start();discover();
}
void Equalizer::discover(){
    if(!m_enabled||qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")||m_discovery.state()!=QProcess::NotRunning)return;
    m_discovery.start("pw-dump",{});m_discoveryTimeout.start();
}
void Equalizer::acceptDiscovery(){
    const auto outputs=Phosrack::parseAudioOutputs(m_discovery.readAllStandardOutput());
    if(m_target.isEmpty()){
        bool exists=false;
        for(const auto &entry:outputs.sources)if(entry.toMap().value("id").toString()=="phosrack.equalizer")exists=true;
        if(exists){fail(tr("Another Phosrack equalizer is already running"));return;}
        if(outputs.defaultId.isEmpty()||outputs.defaultId.startsWith("phosrack.equalizer")){
            fail(tr("Choose a physical default output before enabling the equalizer"));return;
        }
        m_target=outputs.defaultId;launch();return;
    }
    bool targetPresent=false;
    for(const auto &entry:outputs.sources){
        const auto output=entry.toMap();
        const auto name=output.value("id").toString();
        if(name==m_target)targetPresent=true;
        if(name=="phosrack.equalizer"){
            const int id=output.value("objectId",-1).toInt();
            if(id>0&&m_nodeId!=QString::number(id)){
                m_nodeId=QString::number(id);m_startTimeout.stop();
                m_status=tr("Select Phosrack EQ as the output in your system sound settings");
                emit changed();sendControls();
            }
        }
    }
    if(!targetPresent){fail(tr("The equalizer's output disconnected. Choose an output and enable it again."));return;}
}
void Equalizer::launch(){
    m_directory=std::make_unique<QTemporaryDir>(QDir::tempPath()+"/phosrack-eq-XXXXXX");
    if(!m_directory->isValid()){fail(tr("Could not create the equalizer configuration"));return;}
    const auto fileName=m_directory->filePath("equalizer.json");
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)){fail(tr("Could not create the equalizer configuration"));return;}
    const auto config=Phosrack::equalizerConfiguration(m_gains,m_preamp,m_target).toUtf8();
    if(file.write(config)!=config.size()){fail(tr("Could not create the equalizer configuration"));return;}
    file.close();
    auto worker=QCoreApplication::applicationDirPath()+"/phosrack-eq-worker";
    if(!QFile::exists(worker))worker=QStandardPaths::findExecutable("phosrack-eq-worker");
    if(worker.isEmpty()){fail(tr("The equalizer helper is missing. Reinstall Phosrack."));return;}
    m_workerOutput.clear();m_controlsDirty=false;m_starting=true;m_filter.start(worker,{fileName});m_startTimeout.start();
}
void Equalizer::sendControls(){
    if(!running()||m_stopping||!m_controlsDirty||qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    QJsonArray gains;for(const auto &gain:m_gains)gains.append(gain.toDouble());
    m_filter.write(QJsonDocument(QJsonObject{{"gains",gains},{"preamp",m_preamp}}).toJson(QJsonDocument::Compact)+'\n');
    m_controlsDirty=false;
}
void Equalizer::stop(){
    m_stopping=true;m_enabled=false;m_starting=false;m_routes.stop();m_update.stop();m_startTimeout.stop();m_discoveryTimeout.stop();
    for(auto *process:{&m_discovery,&m_filter})if(process->state()!=QProcess::NotRunning){
        process->terminate();if(!process->waitForFinished(400)){process->kill();process->waitForFinished(400);}
    }
    m_nodeId.clear();m_target.clear();m_directory.reset();m_stopping=false;
}
void Equalizer::fail(const QString &message){
    stop();m_error=message;m_status=tr("Equalizer off");emit changed();
}
