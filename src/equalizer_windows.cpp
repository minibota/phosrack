// SPDX-License-Identifier: GPL-3.0-or-later
#include "equalizer.h"
#include "apoconfig.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <array>
#include <algorithm>
#include <cmath>

namespace {
double bounded(double x){return std::isfinite(x)?std::clamp(x,-12.0,12.0):0.0;}
struct Preset {const char *id;std::array<double,10> gains;double preamp;};
constexpr std::array<Preset,7> curves{{
    {"flat",{0,0,0,0,0,0,0,0,0,0},0},
    {"rock",{4,3,1,-1,-2,-1,1,3,4,3},-5},
    {"pop",{-1,1,3,4,3,0,-1,-1,0,1},-5},
    {"jazz",{3,2,0,1,-1,-1,0,1,3,4},-5},
    {"classical",{3,2,1,0,-2,-2,0,1,2,3},-4},
    {"bass",{8,6,4,2,0,0,0,0,0,0},-9},
    {"vocal",{-3,-2,-1,0,2,4,4,2,0,-2},-5}
}};
bool writeFile(const QString &path,const QByteArray &data) {
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly)&&file.write(data)==data.size()&&file.commit();
}
}
Equalizer::Equalizer(QObject *parent):QObject(parent),m_settings("Phosrack","Phosrack"),m_status(tr("Equalizer off")) {
    for(int i=0;i<10;++i)m_gains<<0.;
    const bool test=qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE");
    if(!test) {
        const auto stored=m_settings.value("eq/gains").toList();
        if(stored.size()==10)for(int i=0;i<10;++i)m_gains[i]=bounded(stored[i].toDouble());
        m_preamp=bounded(m_settings.value("eq/preamp",0.).toDouble());
        const auto preset=m_settings.value("eq/preset","flat").toString();
        if(preset=="custom"||std::any_of(curves.begin(),curves.end(),[&](const Preset &p){return preset==p.id;}))m_preset=preset;
    }
    m_apoDirectory=test?QString():m_settings.value("eq/apoDirectory",
        QDir(qEnvironmentVariable("ProgramFiles","C:/Program Files")).filePath("EqualizerAPO/config")).toString();
    m_update.setSingleShot(true);m_update.setInterval(100);
    connect(&m_update,&QTimer::timeout,this,&Equalizer::sendControls);
}
Equalizer::~Equalizer(){stop();}
QVariantList Equalizer::presets() const {
    return {
        QVariantMap{{"id","flat"},{"name",tr("Flat")}},QVariantMap{{"id","rock"},{"name",tr("Rock")}},
        QVariantMap{{"id","pop"},{"name",tr("Pop")}},QVariantMap{{"id","jazz"},{"name",tr("Jazz")}},
        QVariantMap{{"id","classical"},{"name",tr("Classical")}},QVariantMap{{"id","bass"},{"name",tr("Bass boost")}},
        QVariantMap{{"id","vocal"},{"name",tr("Vocal")}},QVariantMap{{"id","custom"},{"name",tr("Custom")}}
    };
}
void Equalizer::save(){
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    m_settings.setValue("eq/gains",m_gains);m_settings.setValue("eq/preamp",m_preamp);m_settings.setValue("eq/preset",m_preset);
}
void Equalizer::setBand(int index,double gain) {
    if(index<0||index>=10||!std::isfinite(gain))return;
    gain=bounded(gain);if(m_gains[index].toDouble()==gain)return;
    m_gains[index]=gain;m_preset="custom";save();emit gainsChanged();m_update.start();
}
void Equalizer::setPreamp(double gain) {
    if(!std::isfinite(gain))return;
    gain=bounded(gain);if(m_preamp==gain)return;
    m_preamp=gain;m_preset="custom";save();emit gainsChanged();m_update.start();
}
void Equalizer::selectPreset(const QString &id) {
    if(id=="custom"){m_preset=id;save();emit gainsChanged();return;}
    for(const auto &curve:curves)if(id==curve.id){
        for(int i=0;i<10;++i)m_gains[i]=curve.gains[i];
        m_preamp=curve.preamp;m_preset=id;save();emit gainsChanged();m_update.start();return;
    }
}
void Equalizer::reset(){selectPreset("flat");}
void Equalizer::setApoDirectory(const QString &path) {
    const auto trimmed=path.trimmed();
    const auto folder=trimmed.isEmpty()?QString():QDir::cleanPath(trimmed);
    if(folder==m_apoDirectory)return;
    if(m_enabled){stop();if(m_enabled){emit changed();return;}}
    m_apoDirectory=folder;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))m_settings.setValue("eq/apoDirectory",folder);
    m_status=tr("Equalizer off");m_error.clear();emit changed();
}
void Equalizer::setEnabled(bool value) {
    if(value==m_enabled)return;
    m_error.clear();
    if(!value){stop();if(!m_enabled)m_status=tr("Equalizer off");emit changed();return;}
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")){
        m_enabled=true;m_status=tr("Equalizer preview — audio unchanged");emit changed();return;
    }
    if(m_apoDirectory.isEmpty()||!QDir::isAbsolutePath(m_apoDirectory)) {
        fail(tr("Install and configure Equalizer APO, then choose its config folder in Settings"));return;
    }
    const auto rootPath=QDir(m_apoDirectory).filePath("config.txt");
    QFile config(rootPath);
    if(!config.open(QIODevice::ReadOnly)){fail(tr("Install and configure Equalizer APO, then choose its config folder in Settings"));return;}
    const auto original=config.readAll();config.close();
    // Preserve the user's existing chain. Add our own clearly marked include,
    // with a backup, only after the user explicitly enables the equalizer.
    const auto attachment=Phosrack::attachApoConfigurationBytes(original);
    if(!attachment.supported) {
        fail(tr("Use UTF-8 or ANSI for Equalizer APO config.txt; UTF-16 is not supported."));return;
    }
    const auto &updated=attachment.contents;
    if(updated!=original){
        const auto backup=rootPath+".phosrack-backup";
        if(!QFileInfo::exists(backup)&&!QFile::copy(rootPath,backup)){fail(tr("Cannot back up Equalizer APO configuration. Check folder permissions."));return;}
    }
    m_enabled=true;
    if(!writeFile(QDir(m_apoDirectory).filePath("phosrack.txt"),Phosrack::apoConfiguration(m_gains,m_preamp,true).toUtf8())||
       (updated!=original&&!writeFile(rootPath,updated))){
        stop();fail(tr("Cannot update Equalizer APO configuration. Check folder permissions."));return;
    }
    m_nodeId="apo";m_status=tr("APO filters updated. Enable Equalizer APO on your output device.");emit changed();
}
void Equalizer::sendControls() {
    if(!m_enabled||qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    if(!writeFile(QDir(m_apoDirectory).filePath("phosrack.txt"),Phosrack::apoConfiguration(m_gains,m_preamp,true).toUtf8())){
        fail(tr("Cannot update Equalizer APO configuration. Check folder permissions."));return;
    }
    m_error.clear();m_status=tr("APO filters updated. Enable Equalizer APO on your output device.");emit changed();
}
void Equalizer::stop() {
    m_update.stop();
    if(m_enabled&&!m_nodeId.isEmpty()&&!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&
       !writeFile(QDir(m_apoDirectory).filePath("phosrack.txt"),Phosrack::apoConfiguration({},0,false).toUtf8())) {
        m_status=tr("APO filters may still be active");
        m_error=tr("Cannot bypass Equalizer APO filters. Check folder permissions or disable the Phosrack include manually.");
        return;
    }
    m_enabled=false;m_nodeId.clear();
}
void Equalizer::fail(const QString &message){
    stop();
    m_error=m_enabled?message+" "+m_error:message;
    if(!m_enabled)m_status=tr("Equalizer off");
    emit changed();
}
