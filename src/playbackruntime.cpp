// SPDX-License-Identifier: GPL-3.0-or-later
#include "playbackruntime.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QVersionNumber>
#include <algorithm>

namespace {
QString configuredPath() {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return {};
    return QSettings("Phosrack","Phosrack").value("playback/widevinePath").toString();
}
QString moduleInVersions(const QString &root,const QString &suffix) {
    auto versions=QDir(root).entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    std::sort(versions.begin(),versions.end(),[](const QString &a,const QString &b){
        return QVersionNumber::fromString(a)>QVersionNumber::fromString(b);
    });
    for(const auto &version:versions) {
        const auto path=QDir(root).filePath(version+'/'+suffix);
        if(QFileInfo(path).isReadable()&&QFileInfo(path).isFile())return path;
    }
    return {};
}
}

PlaybackRuntime::PlaybackRuntime(QObject *parent):QObject(parent),m_initialPath(configuredPath()),m_path(m_initialPath){}
QString PlaybackRuntime::automaticPath() {
#ifdef Q_OS_WIN
    const QString module=QStringLiteral("_platform_specific/win_x64/widevinecdm.dll");
    const QString local=qEnvironmentVariable("LOCALAPPDATA");
    QString path=moduleInVersions(local+"/Google/Chrome/User Data/WidevineCdm",module);
    if(!path.isEmpty())return path;
    for(const auto &root:{qEnvironmentVariable("ProgramFiles"),qEnvironmentVariable("ProgramFiles(x86)"),local}) {
        if(root.isEmpty())continue;
        path=moduleInVersions(root+"/Google/Chrome/Application","WidevineCdm/"+module);
        if(!path.isEmpty())return path;
    }
#else
    const QString suffix=QStringLiteral("WidevineCdm/_platform_specific/linux_x64/libwidevinecdm.so");
    for(const auto &root:{QStringLiteral("/opt/google/chrome/"),QStringLiteral("/run/host/opt/google/chrome/"),QStringLiteral("/usr/lib64/chromium-browser/"),QStringLiteral("/usr/lib/chromium/")}) {
        const auto path=root+suffix;
        if(QFileInfo(path).isReadable()&&QFileInfo(path).isFile())return path;
    }
    const auto path=moduleInVersions(QDir::homePath()+"/.config/google-chrome/WidevineCdm","_platform_specific/linux_x64/libwidevinecdm.so");
    if(!path.isEmpty())return path;
#endif
    return {};
}
QString PlaybackRuntime::detectedPath() const {return m_path.isEmpty()?automaticPath():m_path;}
void PlaybackRuntime::setWidevinePath(const QString &path) {
    const auto cleaned=path.trimmed();
    if(!cleaned.isEmpty()&&(!QFileInfo(cleaned).isFile()||!QFileInfo(cleaned).isReadable()||cleaned.contains('\n')||cleaned.contains('\r')||cleaned.contains('"'))) {
        m_error=tr("Choose a readable Widevine module from an existing trusted installation.");emit changed();return;
    }
    m_error.clear();m_path=cleaned;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")) {
        QSettings settings("Phosrack","Phosrack");
        if(cleaned.isEmpty())settings.remove("playback/widevinePath");
        else settings.setValue("playback/widevinePath",cleaned);
    }
    emit changed();
}
void PlaybackRuntime::chooseWidevine() {
#ifdef Q_OS_WIN
    const auto filter=tr("Widevine module (widevinecdm.dll)");
#else
    const auto filter=tr("Widevine module (libwidevinecdm.so)");
#endif
    const auto path=QFileDialog::getOpenFileName(nullptr,tr("Select an existing Widevine module"),QFileInfo(detectedPath()).absolutePath(),filter);
    if(!path.isEmpty())setWidevinePath(path);
}
void PlaybackRuntime::configureBeforeApplication() {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    const auto path=configuredPath().isEmpty()?automaticPath():configuredPath();
    if(path.isEmpty()||!QFileInfo(path).isReadable()||path.contains('"')||path.contains('\n')||path.contains('\r'))return;
    auto flags=qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    if(flags.contains("--widevine-path"))return;
    flags+=" --widevine-path=\""+QDir::fromNativeSeparators(path).toUtf8()+"\"";
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",flags);
}
