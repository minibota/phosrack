// SPDX-License-Identifier: GPL-3.0-or-later
#include "webplaybackcontroller.h"
#include <QFile>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <cmath>
#include <memory>

namespace {
class PlaybackPage final : public QWebEnginePage {
public:
    PlaybackPage(QWebEngineProfile *profile, const QUrl &url, QObject *parent)
        : QWebEnginePage(profile,parent), m_allowed(url) {}
protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType, bool mainFrame) override {
        // SDK subframes are web content, never native application navigation.
        return mainFrame ? url==m_allowed : (url.scheme()=="https" || url==QUrl("about:blank"));
    }
    QWebEnginePage *createWindow(WebWindowType) override { return nullptr; }
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel, const QString &, int, const QString &) override {
        // Third-party messages can contain auth URLs. Only categorical bridge
        // events below cross into application diagnostics.
    }
private:
    QUrl m_allowed;
};
QByteArray readResource(const QString &name) {
    QFile file(name);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
void respond(QTcpSocket *socket, int code, const QByteArray &body, const QByteArray &type="text/plain; charset=utf-8") {
    const auto reason=code==200?"OK":code==404?"Not Found":code==403?"Forbidden":"Bad Request";
    socket->write("HTTP/1.1 "+QByteArray::number(code)+" "+reason+
        "\r\nContent-Type: "+type+"\r\nContent-Length: "+QByteArray::number(body.size())+
        "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff"
        "\r\nReferrer-Policy: no-referrer\r\nContent-Security-Policy: frame-ancestors 'none'; base-uri 'none'"
        "\r\nConnection: close\r\n\r\n"+body);
    socket->disconnectFromHost();
}
}

class PlaybackBridge final : public QObject {
    Q_OBJECT
public:
    PlaybackBridge(WebPlaybackController *controller, int generation, QObject *parent)
        : QObject(parent),m_controller(controller),m_generation(generation) {}
public slots:
    QString getToken() const {
        return m_controller && m_controller->m_generation==m_generation
            ? QString::fromUtf8(m_controller->m_token) : QString();
    }
    void report(const QString &event, const QVariantMap &data) {
        if(m_controller)m_controller->receiveEvent(m_generation,event,data);
    }
signals:
    void commandRequested(const QString &action, const QVariant &value);
    void tokenUpdated();
private:
    QPointer<WebPlaybackController> m_controller;
    int m_generation;
};

WebPlaybackController::WebPlaybackController(QObject *parent) : QObject(parent) {
    m_status=tr("Connect Spotify to enable local playback.");
    m_startup.setSingleShot(true);
    connect(&m_server,&QTcpServer::newConnection,this,&WebPlaybackController::acceptConnection);
    connect(&m_startup,&QTimer::timeout,this,[this] {
        if(m_page&&!m_ready) {
            setStatus(tr("Local playback unavailable"));
            setError(tr("The Spotify player did not become ready. Check your connection and DRM setup, then retry."));
        }
    });
}
WebPlaybackController::~WebPlaybackController() { stop(); }
void WebPlaybackController::setStatus(const QString &value) {
    if(m_status==value)return;
    m_status=value;emit statusChanged();
}
void WebPlaybackController::setError(const QString &value) {
    if(m_error==value)return;
    m_error=value;emit errorChanged();
}
void WebPlaybackController::setEnabled(bool enabled) {
    if(m_enabled==enabled)return;
    m_enabled=enabled;emit enabledChanged();
    if(enabled)start();
    else {stop();setError({});setStatus(tr("External Spotify player selected."));}
}
void WebPlaybackController::setAccessToken(const QByteArray &token) {
    if(m_token==token)return;
    m_token=token;
    if(token.isEmpty()) {
        stop();setError({});
        setStatus(m_enabled?tr("Connect Spotify to enable local playback."):tr("External Spotify player selected."));
    } else if(m_bridge)emit m_bridge->tokenUpdated();
    else start();
}
void WebPlaybackController::reset() { setAccessToken({}); }
void WebPlaybackController::retry() {
    stop();setError({});start();
}
void WebPlaybackController::unavailable() {
    const bool hadDevice=m_ready||!m_deviceId.isEmpty();
    m_ready=false;m_deviceId.clear();
    if(hadDevice) {emit readyChanged();emit deviceUnavailable();}
    emit playbackState({{"active",false}});
}
void WebPlaybackController::stop() {
    ++m_generation;m_startup.stop();m_server.close();
    const auto sockets=m_sockets;
    for(auto *socket:sockets){socket->abort();socket->deleteLater();}
    m_sockets.clear();
    // A page must be destroyed before its profile. Closing the private page
    // closes the SDK connection and its audio stream without touching other apps.
    delete m_page;m_page=nullptr;m_bridge=nullptr;
    delete m_profile;m_profile=nullptr;
    m_url=QUrl();m_route.clear();m_origin.clear();
    unavailable();
}
void WebPlaybackController::start() {
    if(!m_enabled||m_token.isEmpty()||m_page)return;
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")) {
        setStatus(tr("Local playback is disabled in test mode."));return;
    }
    setError({});setStatus(tr("Starting Spotify playback…"));
    m_html=readResource(QStringLiteral(":/webplayer/player.html"));
    m_channelScript=readResource(QStringLiteral(":/qtwebchannel/qwebchannel.js"));
    if(m_html.isEmpty()||m_channelScript.isEmpty()) {
        setError(tr("The local player resources are missing. Reinstall Phosrack."));return;
    }
    if(!m_server.listen(QHostAddress::LocalHost,0)) {
        setError(tr("Cannot start the local playback service."));return;
    }
    QByteArray nonce;
    for(int i=0;i<8;++i)nonce+=QByteArray::number(QRandomGenerator::system()->generate(),16).rightJustified(8,'0');
    m_route="/"+nonce+"/";
    m_origin="http://127.0.0.1:"+QByteArray::number(m_server.serverPort());
    m_url=QUrl::fromEncoded(m_origin+m_route);
    m_profile=new QWebEngineProfile(this); // No storage name: off-the-record.
    m_profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    m_page=new PlaybackPage(m_profile,m_url,this);
    // Native QML buttons initiate playback; there is no HTML button receiving
    // the user's gesture. This does not bypass DRM or change sandbox/TLS policy.
    m_page->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture,false);
    m_page->setVisible(true);
    m_page->setLifecycleState(QWebEnginePage::LifecycleState::Active);
    auto *channel=new QWebChannel(m_page);
    m_bridge=new PlaybackBridge(this,m_generation,m_page);
    channel->registerObject(QStringLiteral("playback"),m_bridge);
    m_page->setWebChannel(channel);
    const int generation=m_generation;
    connect(m_page,&QWebEnginePage::loadFinished,this,[this,generation](bool ok) {
        if(generation!=m_generation)return;
        if(!ok){m_startup.stop();setError(tr("Could not load the local Spotify player. Retry playback."));}
    });
    connect(m_page,&QWebEnginePage::renderProcessTerminated,this,[this,generation](QWebEnginePage::RenderProcessTerminationStatus,int) {
        if(generation!=m_generation)return;
        m_startup.stop();unavailable();setStatus(tr("Local playback unavailable"));
        setError(tr("The local playback process stopped. Retry playback."));
    });
    m_page->load(m_url);m_startup.start(45000);
}
void WebPlaybackController::acceptConnection() {
    while(m_server.hasPendingConnections()) {
        auto *socket=m_server.nextPendingConnection();socket->setParent(this);m_sockets.insert(socket);
        connect(socket,&QObject::destroyed,this,[this,socket]{m_sockets.remove(socket);});
        connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        QTimer::singleShot(5000,socket,[socket]{socket->abort();socket->deleteLater();});
        struct Request {QByteArray bytes;bool done=false;};
        auto request=std::make_shared<Request>();const int generation=m_generation;
        connect(socket,&QTcpSocket::readyRead,this,[this,socket,request,generation] {
            if(request->done)return;
            request->bytes+=socket->readAll();
            if(generation!=m_generation){socket->abort();return;}
            if(request->bytes.size()>8192){request->done=true;respond(socket,400,"Request too large");return;}
            const auto end=request->bytes.indexOf("\r\n\r\n");if(end<0)return;
            request->done=true;
            const auto lines=request->bytes.left(end).split('\n');
            const auto first=lines.value(0).trimmed().split(' ');
            if(first.size()!=3||first[0]!="GET"||(first[2]!="HTTP/1.1"&&first[2]!="HTTP/1.0")) {
                respond(socket,400,"Invalid request");return;
            }
            QByteArray host,origin;int hostCount=0,originCount=0;
            for(int i=1;i<lines.size();++i) {
                const auto line=lines[i].trimmed();const auto colon=line.indexOf(':');
                if(colon<=0){respond(socket,400,"Invalid header");return;}
                const auto key=line.left(colon).toLower(), value=line.mid(colon+1).trimmed();
                if(key=="host"){host=value;++hostCount;}
                if(key=="origin"){origin=value;++originCount;}
            }
            if(hostCount!=1||host!="127.0.0.1:"+QByteArray::number(m_server.serverPort())||originCount>1||(!origin.isEmpty()&&origin!=m_origin)) {
                respond(socket,403,"Invalid origin");return;
            }
            if(first[1]==m_route)respond(socket,200,m_html,"text/html; charset=utf-8");
            else if(first[1]==m_route+"qwebchannel.js")respond(socket,200,m_channelScript,"application/javascript; charset=utf-8");
            else respond(socket,404,"Not found");
        });
    }
}
void WebPlaybackController::command(const QString &action,const QVariant &value) {
    static const QSet<QString> allowed={"activate","pause","resume","next","previous","seek","volume"};
    if(!allowed.contains(action)||!m_ready||!m_bridge){emit commandFinished(action,false);return;}
    QVariant argument=value;
    if(action=="volume"||action=="seek") {
        bool valid=false;const double number=value.toDouble(&valid);
        if(!valid||!std::isfinite(number)){emit commandFinished(action,false);return;}
        argument=action=="volume"?qBound(0.0,number,100.0):qBound(0.0,number,86400000.0);
    }
    emit m_bridge->commandRequested(action,argument);
}
QVariantMap WebPlaybackController::nativeState(const QVariantMap &data,const QString &deviceId) {
    QVariantMap state;
    // Keep only fields used by the native player; nothing is logged.
    for(const auto &key:{"active","playing","position","duration","volume","uri","title","artist","album","art","url","shuffle","repeat","canControl","canToggle","canNext","canPrevious","canSeek","canVolume","canShuffle","canRepeat"})
        if(data.contains(key))state.insert(key,data.value(key));
    static const QRegularExpression collection("^spotify:(album|playlist):[A-Za-z0-9]{22}$");
    static const QRegularExpression playable("^spotify:(track|episode):[A-Za-z0-9]{22}$");
    const auto context=data.value("contextUri").toString();
    if(collection.match(context).hasMatch())state["contextUri"]=context;
    QVariantList next;
    for(const auto &entry:data.value("nextTracks").toList()) {
        if(next.size()>=20)break;
        const auto item=entry.toMap();
        const auto uri=item.value("uri").toString();
        const auto title=item.value("title").toString();
        if(title.isEmpty()||!playable.match(uri).hasMatch())continue;
        const auto art=QUrl(item.value("art").toString());
        next.append(QVariantMap{{"uri",uri},{"title",title.left(500)},
            {"subtitle",item.value("subtitle").toString().left(500)},
            {"art",art.scheme()=="https"?art.toString():QString()},{"canPlay",true}});
    }
    state["nextTracks"]=next;
    state["device"]=QStringLiteral("Phosrack");state["deviceId"]=deviceId;
    return state;
}
void WebPlaybackController::receiveEvent(int generation,const QString &event,const QVariantMap &data) {
    if(generation!=m_generation||!m_enabled||m_token.isEmpty()||!m_page)return;
    if(event=="ready") {
        const auto id=data.value("deviceId").toString();
        if(!QRegularExpression("^[A-Za-z0-9_-]{1,128}$").match(id).hasMatch())return;
        m_startup.stop();m_ready=true;m_deviceId=id;setError({});setStatus(tr("Ready to play on this computer"));
        emit readyChanged();emit deviceReady(id);
    } else if(event=="not_ready") {
        unavailable();setStatus(tr("Spotify playback is reconnecting…"));m_startup.start(45000);
    } else if(event=="state") {
        if(!m_ready)return;
        emit playbackState(nativeState(data,m_deviceId));
    } else if(event=="command_finished") {
        const auto action=data.value("action").toString();const bool ok=data.value("ok").toBool();
        emit commandFinished(action,ok);
        if(!ok)setError(tr("Spotify could not complete this playback control. Try again."));
        else if(m_ready){setError({});setStatus(tr("Ready to play on this computer"));}
    } else if(event=="capabilities") {
        QVariantMap available;
        for(const auto &key:{"secureContext","eme","aac","vorbis","opus","widevine"})
            available.insert(key,data.value(key).toBool());
        emit capabilities(available);
    } else {
        QString message;
        if(event=="drm_unavailable")message=tr("Widevine DRM or a required audio codec is unavailable. Configure Widevine in Settings, then restart Phosrack.");
        else if(event=="sdk_load_error")message=tr("Could not download the Spotify playback SDK. Check your connection and retry.");
        else if(event=="initialization_error")message=tr("Spotify playback could not initialize. Check Widevine and audio codec support.");
        else if(event=="codec_initialization_error")message=tr("Spotify playback could not initialize and this build lacks AAC support. Use another package or select an external Spotify player.");
        else if(event=="authentication_error")message=tr("Spotify playback authorization failed. Reconnect Spotify to grant streaming access.");
        else if(event=="account_error")message=tr("Spotify Premium is required for integrated playback.");
        else if(event=="playback_error")message=tr("Spotify could not play this track. Try another track or retry playback.");
        else if(event=="autoplay_failed")message=tr("Spotify could not start audio. Press Play again.");
        else if(event=="connection_error")message=tr("Could not connect the Spotify player. Check your connection and retry.");
        if(message.isEmpty())return;
        m_startup.stop();
        if(event!="playback_error"&&event!="autoplay_failed")unavailable();
        setError(message);setStatus(tr("Local playback unavailable"));
    }
}
#include "webplaybackcontroller.moc"
