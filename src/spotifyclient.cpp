// SPDX-License-Identifier: GPL-3.0-or-later
#include "spotifyclient.h"
#include "protocol.h"
#include "spotifyconfig.h"
#ifdef Q_OS_WIN
#include "tokenstore.h"
#endif
#include <QDateTime>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QUrlQuery>
#include <algorithm>
#include <memory>

namespace {
const QString redirectUri=QStringLiteral("http://127.0.0.1:43821/callback");
QByteArray formBody(const QList<QPair<QString,QString>> &items) {
    QByteArray body;
    for(const auto &item:items) {
        if(!body.isEmpty())body+='&';
        body+=QUrl::toPercentEncoding(item.first)+'='+QUrl::toPercentEncoding(item.second);
    }
    return body;
}
QVariantMap catalogItem(QJsonObject item) {
    // Library and playlist pages wrap entities. Both the current `item` and
    // older `track` shape are accepted without inventing unavailable entries.
    for(const auto &field:{"item","track","album"}) {
        if(item.contains("uri"))break;
        if(item.value(field).isObject())item=item.value(field).toObject();
    }
    const QString uri=item.value("uri").toString();
    if(uri.isEmpty()||item.value("is_local").toBool())return {};
    const QString type=item.value("type").toString(uri.section(':',1,1));
    QStringList artists;
    for(const auto &artist:item.value("artists").toArray())artists<<artist.toObject().value("name").toString();
    QString subtitle=artists.join(", ");
    if(subtitle.isEmpty())subtitle=item.value("show").toObject().value("name").toString();
    if(subtitle.isEmpty())subtitle=item.value("owner").toObject().value("display_name").toString();
    auto images=item.value("images").toArray();
    if(images.isEmpty())images=item.value("album").toObject().value("images").toArray();
    return {{"id",item.value("id").toString(uri.section(':',2,2))},{"uri",uri},
        {"title",item.value("name").toString()},{"subtitle",subtitle},{"type",type},
        {"art",images.isEmpty()?QString():images.last().toObject().value("url").toString()},
        {"contextUri",type=="album"||type=="playlist"?uri:QString()},
        {"canPlay",item.value("is_playable").toBool(true)&&item.value("restrictions").toObject().isEmpty()}};
}
QVariantList catalogItems(const QJsonArray &items) {
    QVariantList result;
    for(const auto &item:items) {
        const auto normalized=catalogItem(item.toObject());
        if(!normalized.isEmpty())result<<normalized;
    }
    return result;
}
bool playbackUri(const QString &uri,bool context) {
    static const QRegularExpression tracks("^spotify:(track|episode):[A-Za-z0-9]{22}$");
    static const QRegularExpression collections("^spotify:(album|playlist):[A-Za-z0-9]{22}$");
    return (context?collections:tracks).match(uri).hasMatch();
}
}
SpotifyClient::SpotifyClient(QObject *parent, QNetworkAccessManager *transport)
    :QObject(parent),m_network(transport ? transport : new QNetworkAccessManager(this)),m_settings("Phosrack","Phosrack"),
      m_defaultClientId(QString::fromLatin1(Phosrack::BuildConfig::SpotifyClientId)) {
    m_clientId=m_defaultClientId;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")) {
        const auto saved=m_settings.value("spotify/clientId").toString().trimmed();
        if(!saved.isEmpty())m_clientId=saved;
        m_integratedPlayback=m_settings.value("spotify/integratedPlayback",true).toBool();
        m_streamingAuthorized=m_settings.value(streamingGrantKey(),false).toBool();
    }
    m_track=Phosrack::playbackState({});
    m_status=tr("Not connected");
    m_poll.setInterval(3000);
    connect(&m_poll,&QTimer::timeout,this,&SpotifyClient::refreshState);
    m_refresh.setSingleShot(true);
    connect(&m_refresh,&QTimer::timeout,this,&SpotifyClient::refreshToken);
    m_loginTimeout.setSingleShot(true);
    connect(&m_loginTimeout,&QTimer::timeout,this,[this]{
        m_callback.close();m_oauthState.clear();m_verifier.clear();m_connecting=false;
        m_status=tr("Not connected");fail(tr("Sign-in timed out. Please connect again."));emit sessionChanged();
    });
    connect(&m_callback,&QTcpServer::newConnection,this,&SpotifyClient::acceptCallback);
    m_clock.setInterval(500);
    connect(&m_clock,&QTimer::timeout,this,[this]{if(track().value("playing").toBool())emit trackChanged();});
    m_clock.start();
}
SpotifyClient::~SpotifyClient() {
#ifndef Q_OS_WIN
    if(m_wallet)delete m_wallet;
#endif
}
void SpotifyClient::setClientId(const QString &id) {
    const QString cleaned=id.trimmed();
    const QString effective=cleaned.isEmpty()?m_defaultClientId:cleaned;
    if(effective==m_clientId){saveClientIdOverride();return;}
    // Changing application profiles cancels only the active in-memory session.
    // Stored refresh tokens remain keyed by Client ID for a later switch back.
    clearSession(false);m_clientId=effective;saveClientIdOverride();
    m_streamingAuthorized=!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&m_settings.value(streamingGrantKey(),false).toBool();
    emit clientIdChanged();
}
void SpotifyClient::resetToDefaultClientId(){setClientId({});}
void SpotifyClient::saveClientIdOverride() {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    if(m_clientId==m_defaultClientId)m_settings.remove("spotify/clientId");
    else m_settings.setValue("spotify/clientId",m_clientId);
}
QVariantMap SpotifyClient::track() const {
    auto value=m_integratedPlayback
        ?(m_localPlaybackActive?m_localTrack:Phosrack::playbackState({}))
        :m_track;
    if(m_integratedPlayback) {
        value["volume"]=m_localVolume;
        value["device"]=localPlaybackReady()?QStringLiteral("Phosrack"):QString();
        value["deviceId"]=m_localDeviceId;
        value["canToggle"]=localPlaybackReady()&&(!m_localPlaybackActive||value.value("canToggle").toBool());
        value["canVolume"]=localPlaybackReady();
    }
    const auto &clock=m_integratedPlayback?m_localPositionClock:m_positionClock;
    if(value.value("playing").toBool()&&clock.isValid())
        value["position"]=std::min(value.value("duration").toInt(),value.value("position").toInt()+int(clock.elapsed()));
    return value;
}
void SpotifyClient::setIntegratedPlayback(bool value) {
    if(m_integratedPlayback==value)return;
    m_integratedPlayback=value;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))m_settings.setValue("spotify/integratedPlayback",value);
    clearLocalPlaybackDevice();
    if(value&&authorized()&&!m_streamingAuthorized)requireStreamingAuthorization();
    emit integratedPlaybackChanged();emit trackChanged();
}
QString SpotifyClient::streamingGrantKey() const {return "spotify/streamingGrant/"+m_clientId;}
bool SpotifyClient::hasStreamingGrant(const QString &scopes) const {
    const auto granted=scopes.split(' ',Qt::SkipEmptyParts);
    return granted.contains("streaming")&&granted.contains("user-read-private")&&granted.contains("user-read-email");
}
void SpotifyClient::requireStreamingAuthorization() {
    // Preserve the previous protected token until a new grant replaces it.
    clearSession(false);
    m_reauthorizeStreaming=true;m_streamingAuthorized=false;
    m_status=tr("Please reconnect");
    fail(tr("Connect Spotify again to allow integrated playback. Enable Web Playback SDK in your developer application."));
    emit accessTokenChanged();emit sessionChanged();
}
void SpotifyClient::setLocalPlaybackDevice(const QString &deviceId) {
    if(!m_integratedPlayback||!authorized()||!QRegularExpression("^[A-Za-z0-9_-]{1,128}$").match(deviceId).hasMatch())return;
    if(m_localDeviceId==deviceId)return;
    m_localDeviceId=deviceId;m_localPlaybackActive=false;m_localTrack.clear();
    emit localPlaybackReadyChanged();emit trackChanged();
}
void SpotifyClient::clearLocalPlaybackDevice() {
    const bool hadDevice=!m_localDeviceId.isEmpty();
    m_localDeviceId.clear();m_localTrack.clear();m_localPlaybackActive=false;
    if(hadDevice)emit localPlaybackReadyChanged();
    emit trackChanged();
}
void SpotifyClient::updateLocalPlaybackState(const QVariantMap &state) {
    if(!localPlaybackReady()||!authorized())return;
    if(state.contains("volume"))m_localVolume=std::clamp(state.value("volume").toInt(),0,100);
    if(!state.value("active",true).toBool()) {
        m_localPlaybackActive=false;emit trackChanged();return;
    }
    m_localTrack=state;
    const bool remoteActive=!m_track.value("deviceId").toString().isEmpty()&&m_track.value("deviceId").toString()!=m_localDeviceId;
    if(!remoteActive||state.value("playing").toBool()||m_localPlaybackActive)m_localPlaybackActive=true;
    m_localPositionClock.restart();emit trackChanged();
}
void SpotifyClient::setActive(bool value) {
    if(m_active==value)return;
    m_active=value;m_poll.setInterval(value?3000:30000);emit activeChanged();
    if(value&&authorized())refreshState();
}
void SpotifyClient::fail(const QString &message) {m_error=message;emit errorChanged();}
void SpotifyClient::clearError() {m_error.clear();emit errorChanged();}
void SpotifyClient::connectSpotify() {
    if(m_connecting||authorized())return;
    if(!QRegularExpression("^[a-fA-F0-9]{32}$").match(m_clientId).hasMatch()) {
        fail(tr("Enter the 32-character Client ID of your Spotify application."));return;
    }
    clearError();m_connecting=true;
    if(m_reauthorizeStreaming){beginOAuth();return;}
#ifdef Q_OS_WIN
    m_status=tr("Opening Windows Credential Manager…");emit sessionChanged();
    const int generation=m_generation;
    QTimer::singleShot(0,this,[this,generation]{
        if(generation!=m_generation)return;
        const auto saved=Phosrack::NativeTokenStore::readRefreshToken(m_clientId);
        if(saved.status==Phosrack::NativeTokenStore::ReadStatus::Found) {
            m_refreshToken=saved.token;m_status=tr("Restoring session…");emit sessionChanged();refreshToken();
        } else {
            if(saved.status==Phosrack::NativeTokenStore::ReadStatus::Unavailable)
                fail(tr("Windows Credential Manager is unavailable. This session will only be kept in memory."));
            beginOAuth();
        }
    });
#else
    m_status=tr("Opening KWallet…");emit sessionChanged();
    if(m_wallet&&m_wallet->isOpen()) {withWallet();return;}
    if(m_wallet)delete m_wallet;
    m_wallet=KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),0,KWallet::Wallet::Asynchronous);
    if(!m_wallet) {fail(tr("KWallet is unavailable. This session will only be kept in memory."));beginOAuth();return;}
    const int generation=m_generation;
    connect(m_wallet,&KWallet::Wallet::walletOpened,this,[this,generation](bool open){
        if(generation!=m_generation)return;
        if(open)withWallet();else {fail(tr("KWallet was not opened. This session will only be kept in memory."));beginOAuth();}
    });
#endif
}
#ifndef Q_OS_WIN
void SpotifyClient::withWallet() {
    if(!m_wallet->hasFolder("Phosrack"))m_wallet->createFolder("Phosrack");
    if(!m_wallet->setFolder("Phosrack")) {beginOAuth();return;}
    QString token;
    if(m_wallet->readPassword("spotify:"+m_clientId,token)==0&&!token.isEmpty()) {
        m_refreshToken=token.toUtf8();m_status=tr("Restoring session…");emit sessionChanged();refreshToken();
    } else beginOAuth();
}
#endif
void SpotifyClient::beginOAuth() {
    m_callback.close();
    if(!m_callback.listen(QHostAddress::LocalHost,43821)) {
        m_connecting=false;m_status=tr("Not connected");
        fail(tr("Cannot open 127.0.0.1:43821. Close another Phosrack sign-in and try again."));emit sessionChanged();return;
    }
    m_verifier=Phosrack::randomToken();m_oauthState=Phosrack::randomToken();
    QUrl url("https://accounts.spotify.com/authorize");
    QUrlQuery query;
    query.addQueryItem("response_type","code");query.addQueryItem("client_id",m_clientId);
    query.addQueryItem("redirect_uri",redirectUri);query.addQueryItem("state",QString::fromLatin1(m_oauthState));
    query.addQueryItem("code_challenge_method","S256");
    query.addQueryItem("code_challenge",QString::fromLatin1(Phosrack::pkceChallenge(m_verifier)));
    const QString scopes=QStringLiteral("user-read-playback-state user-modify-playback-state user-read-currently-playing user-library-read playlist-read-private playlist-read-collaborative");
    m_oauthStreamingRequested=m_integratedPlayback;
    query.addQueryItem("scope",m_oauthStreamingRequested?QStringLiteral("streaming user-read-private user-read-email ")+scopes:scopes);
    url.setQuery(query);m_status=tr("Authorize Phosrack in your browser…");emit sessionChanged();m_loginTimeout.start(180000);
    if(!QDesktopServices::openUrl(url)) {
        m_callback.close();m_loginTimeout.stop();m_connecting=false;m_status=tr("Not connected");
        fail(tr("Could not open the default browser."));emit sessionChanged();
    }
}
void SpotifyClient::acceptCallback() {
    while(m_callback.hasPendingConnections()) {
        QTcpSocket *socket=m_callback.nextPendingConnection();
        socket->setParent(this);
        auto buffer=std::make_shared<QByteArray>();
        const int generation=m_generation;
        QTimer::singleShot(8000,socket,[socket]{socket->disconnectFromHost();});
        connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
        connect(socket,&QTcpSocket::readyRead,this,[this,socket,buffer,generation]{
            buffer->append(socket->readAll());
            if(buffer->size()>8192){socket->disconnectFromHost();return;}
            if(!buffer->contains("\r\n\r\n"))return;
            const auto result=Phosrack::parseCallback(*buffer,QString::fromLatin1(m_oauthState));
            const bool valid=result.valid&&generation==m_generation;
            const QByteArray message=(valid?tr("Phosrack: you can close this tab and return to the player."):tr("Invalid request.")).toUtf8();
            socket->write(QByteArray("HTTP/1.1 ")+(valid?"200 OK":"400 Bad Request")+
                "\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: "+QByteArray::number(message.size())+"\r\n\r\n"+message);
            socket->disconnectFromHost();
            if(!valid)return;
            m_callback.close();m_loginTimeout.stop();m_oauthState.clear();
            if(!result.error.isEmpty()) {
                m_verifier.clear();m_connecting=false;m_status=tr("Not connected");
                fail(tr("Spotify authorization was cancelled or denied."));emit sessionChanged();return;
            }
            const auto body=formBody({{"grant_type","authorization_code"},{"code",result.code},
                {"redirect_uri",redirectUri},{"client_id",m_clientId},{"code_verifier",QString::fromLatin1(m_verifier)}});
            m_verifier.clear();requestToken(body);
        });
    }
}
void SpotifyClient::persistRefresh() {
#ifdef Q_OS_WIN
    if(!m_refreshToken.isEmpty()&&!Phosrack::NativeTokenStore::writeRefreshToken(m_clientId,m_refreshToken))
        fail(tr("Could not save the session in Windows Credential Manager. It remains available until Phosrack closes."));
#else
    if(m_wallet&&m_wallet->isOpen()&&!m_refreshToken.isEmpty()) {
        m_wallet->setFolder("Phosrack");
        if(m_wallet->writePassword("spotify:"+m_clientId,QString::fromUtf8(m_refreshToken))!=0)
            fail(tr("Could not save the session in KWallet. It remains available until Phosrack closes."));
    }
#endif
}
void SpotifyClient::forgetRefresh() {
#ifdef Q_OS_WIN
    Phosrack::NativeTokenStore::removeRefreshToken(m_clientId);
#else
    if(m_wallet&&m_wallet->isOpen()) {
        m_wallet->setFolder("Phosrack");m_wallet->removeEntry("spotify:"+m_clientId);
    }
#endif
}
void SpotifyClient::refreshToken() {
    if(m_refreshing||m_refreshToken.isEmpty())return;
    requestToken(formBody({{"grant_type","refresh_token"},{"refresh_token",QString::fromUtf8(m_refreshToken)},{"client_id",m_clientId}}));
}
void SpotifyClient::requestToken(const QByteArray &form) {
    m_refreshing=true;
    QNetworkRequest request(QUrl("https://accounts.spotify.com/api/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");request.setTransferTimeout(15000);
    auto *reply=m_network->post(request,form);
    const int generation=m_generation;
    const bool newGrant=QUrlQuery(QString::fromLatin1(form)).queryItemValue("grant_type")=="authorization_code";
    const bool requestedStreamingGrant=newGrant&&m_oauthStreamingRequested;
    connect(reply,&QNetworkReply::finished,this,[this,reply,generation,newGrant,requestedStreamingGrant]{
        reply->deleteLater();if(generation!=m_generation)return;
        m_refreshing=false;m_connecting=false;
        const int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto data=QJsonDocument::fromJson(reply->readAll()).object();
        if(code!=200||data.value("access_token").toString().isEmpty()) {
            if(data.value("error").toString()=="invalid_grant") {
                forgetRefresh();
                clearSession(false);
                m_status=tr("Please reconnect");fail(tr("Authorization expired or was revoked. Connect Spotify again."));
            } else {
                m_status=authorized()?tr("Cannot reach Spotify"):tr("Not connected");
                fail(tr("Could not obtain a Spotify session. Check the connection, Client ID and redirect URI."));
                if(!m_refreshToken.isEmpty())m_refresh.start(30000);
            }
            emit sessionChanged();return;
        }
        const bool streamingGrant=data.contains("scope")?hasStreamingGrant(data.value("scope").toString()):(newGrant?requestedStreamingGrant:m_streamingAuthorized);
        m_streamingAuthorized=streamingGrant;
        if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))m_settings.setValue(streamingGrantKey(),streamingGrant);
        if(m_integratedPlayback&&!streamingGrant){requireStreamingAuthorization();return;}
        m_accessToken=data.value("access_token").toString().toUtf8();
        m_reauthorizeStreaming=false;
        const auto renewed=data.value("refresh_token").toString();if(!renewed.isEmpty())m_refreshToken=renewed.toUtf8();
        persistRefresh();m_status=tr("Connected to Spotify");
        m_refresh.start(std::max(30,data.value("expires_in").toInt(3600)-60)*1000);
        m_poll.start();emit accessTokenChanged();emit sessionChanged();refreshState();refreshDevices();
    });
}
void SpotifyClient::disconnectSpotify() {
    clearSession(true);
}
void SpotifyClient::clearSession(bool forgetStoredToken) {
    ++m_generation;m_callback.close();m_loginTimeout.stop();m_poll.stop();m_refresh.stop();
    if(forgetStoredToken)forgetRefresh();
    m_accessToken.clear();m_refreshToken.clear();m_oauthState.clear();m_verifier.clear();
    m_reauthorizeStreaming=false;m_oauthStreamingRequested=false;clearLocalPlaybackDevice();emit accessTokenChanged();
    m_connecting=m_refreshing=m_polling=m_command=false;m_backoffUntil=0;
    m_track=Phosrack::playbackState({});m_devices.clear();clearCollections();m_status=tr("Not connected");clearError();
    emit sessionChanged();emit trackChanged();emit devicesChanged();
}
void SpotifyClient::api(const QByteArray &method,const QString &path,const QJsonObject &body,Result done,bool command,std::function<void()> failed,std::function<bool()> current) {
    if(!authorized()||m_refreshing) {if(command)fail(tr("Connect Spotify or wait for the session to refresh."));if(failed)failed();return;}
    if(QDateTime::currentMSecsSinceEpoch()<m_backoffUntil) {if(command)fail(tr("Spotify requested a pause. Please wait before sending another command."));if(failed)failed();return;}
    if(command&&m_command){if(failed)failed();return;}
    if(command){m_command=true;emit sessionChanged();}
    QNetworkRequest request(QUrl("https://api.spotify.com/v1/"+path));
    request.setRawHeader("Authorization","Bearer "+m_accessToken);request.setTransferTimeout(12000);
    if(!body.isEmpty())request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    auto *reply=m_network->sendCustomRequest(request,method,body.isEmpty()?QByteArray():QJsonDocument(body).toJson(QJsonDocument::Compact));
    const int generation=m_generation;
    connect(reply,&QNetworkReply::finished,this,[this,reply,generation,command,done,path,method,failed,current]{
        reply->deleteLater();if(generation!=m_generation)return;
        if(current&&!current())return;
        if(path=="me/player" && method=="GET")m_polling=false;
        if(command){m_command=false;emit sessionChanged();}
        const int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto data=QJsonDocument::fromJson(reply->readAll()).object();
        if(code==401) {refreshToken();if(command)fail(tr("Refreshing session; try the control again when it finishes."));if(failed)failed();return;}
        if(code==429) {
            bool ok=false;int seconds=reply->rawHeader("Retry-After").toInt(&ok);
            if(!ok)seconds=30;
            seconds=std::clamp(seconds,1,86400);
            m_backoffUntil=QDateTime::currentMSecsSinceEpoch()+qint64(seconds)*1000;
            fail(tr("Spotify limit: wait %1 seconds before trying again.").arg(seconds));if(failed)failed();return;
        }
        if(code<200||code>=300) {
            if(code==404)fail(path.startsWith("me/player")
                ?(m_integratedPlayback?tr("Choose a track from Search or Library to start playback in Phosrack."):tr("No Spotify device is active. Open Spotify and play a track first."))
                :tr("This Spotify collection is unavailable."));
            else if(code==403)fail(tr("Spotify denied access. Check that your account is allowed to use this developer application, along with Premium, permissions and the selected device. Reconnect to grant library access."));
            else fail(tr("Could not contact Spotify. Playback state will be retried."));
            if(failed)failed();
            return;
        }
        if(!m_error.contains("KWallet")&&!m_error.contains("Windows"))clearError();
        if(done)done(data,code);
    });
}
void SpotifyClient::refreshState() {
    if(m_polling||!authorized()||m_refreshing||QDateTime::currentMSecsSinceEpoch()<m_backoffUntil)return;
    m_polling=true;
    api("GET","me/player",{},[this](const QJsonObject &o,int){
        m_track=Phosrack::playbackState(o);m_positionClock.restart();
        if(localPlaybackReady()) {
            m_localPlaybackActive=m_track.value("deviceId").toString()==m_localDeviceId;
            if(m_localPlaybackActive) {
                m_localTrack=m_track;
                m_localTrack["canVolume"]=true;
                m_localPositionClock.restart();
            }
        }
        emit trackChanged();
    });
}
void SpotifyClient::refreshDevices() {
    api("GET","me/player/devices",{},[this](const QJsonObject &o,int){
        m_devices.clear();for(const auto &value:o.value("devices").toArray()) {
            const auto d=value.toObject();if(d.value("id").toString().isEmpty())continue;
            m_devices<<QVariantMap{{"id",d.value("id").toString()},{"name",d.value("name").toString()},
                {"active",d.value("is_active").toBool()},{"restricted",d.value("is_restricted").toBool()}};
        } emit devicesChanged();
    });
}
void SpotifyClient::control(const QByteArray &method,const QString &path,const QJsonObject &body) {
    if(!playbackCommandAllowed())return;
    if(m_integratedPlayback&&path=="me/player/play")emit localPlaybackCommand("activate",{});
    api(method,playbackPath(path),body,[this](const QJsonObject &,int){const int g=m_generation;QTimer::singleShot(450,this,[this,g]{if(g==m_generation)refreshState();});},true);
}
bool SpotifyClient::playbackCommandAllowed() {
    if(!authorized()||m_refreshing){fail(tr("Connect Spotify or wait for the session to refresh."));return false;}
    if(m_integratedPlayback&&!localPlaybackReady()) {
        fail(tr("The integrated player is not ready. Check its status in Settings or choose external playback."));return false;
    }
    return true;
}
QString SpotifyClient::playbackPath(const QString &path) const {
    if(!m_integratedPlayback||m_localDeviceId.isEmpty()||path=="me/player")return path;
    return path+(path.contains('?')?"&":"?")+"device_id="+QString::fromLatin1(QUrl::toPercentEncoding(m_localDeviceId));
}
void SpotifyClient::playPause(){
    if(!playbackCommandAllowed())return;
    const auto current=track();
    if(m_integratedPlayback&&!m_localPlaybackActive) {
        if(m_track.value("deviceId").toString().isEmpty()) {
            fail(tr("Choose a track from Search or Library to start playback in Phosrack."));return;
        }
        emit localPlaybackCommand("activate",{});
        control("PUT","me/player",{{"device_ids",QJsonArray{m_localDeviceId}},{"play",true}});return;
    }
    if(current.value("canToggle").toBool())control("PUT",current.value("playing").toBool()?"me/player/pause":"me/player/play");
}
void SpotifyClient::next(){if(track().value("canNext").toBool())control("POST","me/player/next");}
void SpotifyClient::previous(){if(track().value("canPrevious").toBool())control("POST","me/player/previous");}
void SpotifyClient::seek(int ms){const auto current=track();if(current.value("canSeek").toBool())control("PUT","me/player/seek?position_ms="+QString::number(std::clamp(ms,0,current.value("duration").toInt())));}
void SpotifyClient::setVolume(int value){
    if(!track().value("canVolume").toBool()||!playbackCommandAllowed())return;
    const int volume=std::clamp(value,0,100);
    if(m_integratedPlayback) {
        emit localPlaybackCommand("volume",volume);
        m_localVolume=volume;emit trackChanged();
    } else control("PUT","me/player/volume?volume_percent="+QString::number(volume));
}
void SpotifyClient::toggleShuffle(){const auto current=track();if(current.value("canShuffle").toBool())control("PUT",QString("me/player/shuffle?state=")+(current.value("shuffle").toBool()?"false":"true"));}
void SpotifyClient::cycleRepeat(){const auto current=track();if(current.value("canRepeat").toBool()){const auto mode=current.value("repeat").toString();control("PUT","me/player/repeat?state="+(mode=="off"?QString("context"):mode=="context"?QString("track"):QString("off")));}}
void SpotifyClient::transfer(const QString &deviceId){
    for(const auto &value:m_devices){const auto d=value.toMap();
        if(d.value("id").toString()==deviceId&&!d.value("restricted").toBool()){
            if(m_integratedPlayback&&deviceId!=m_localDeviceId) {
                fail(tr("Choose external playback in Settings before selecting another Spotify device."));return;
            }
            if(m_integratedPlayback)emit localPlaybackCommand("activate",{});
            control("PUT","me/player",{{"device_ids",QJsonArray{deviceId}},{"play",true}});return;
        }
    }
}

void SpotifyClient::clearCollections() {
    ++m_searchGeneration;++m_libraryGeneration;
    m_searchResults.clear();m_queueItems.clear();m_libraryItems.clear();
    m_searchQuery.clear();m_libraryView.clear();m_libraryPath.clear();
    m_searchOffset=m_libraryOffset=0;
    m_searchLoading=m_queueLoading=m_libraryLoading=false;
    m_hasMoreSearch=m_hasMoreLibrary=false;
    emit searchChanged();emit queueChanged();emit libraryChanged();emit collectionsChanged();
}
void SpotifyClient::search(const QString &query,bool more) {
    const auto cleaned=query.trimmed().left(256);
    if(more&&(cleaned!=m_searchQuery||m_searchLoading||!m_hasMoreSearch))return;
    const int generation=++m_searchGeneration;
    if(!more) {
        m_searchQuery=cleaned;m_searchOffset=0;m_searchResults.clear();m_hasMoreSearch=false;
        emit searchChanged();
    }
    m_searchLoading=!cleaned.isEmpty();emit collectionsChanged();
    if(cleaned.isEmpty())return;
    // Development-mode search has a maximum of ten results per content type.
    const QString path="search?q="+QString::fromLatin1(QUrl::toPercentEncoding(cleaned))+
        "&type=track,album,playlist&limit=10&offset="+QString::number(m_searchOffset);
    api("GET",path,{},[this,generation](const QJsonObject &page,int){
        if(generation!=m_searchGeneration)return;
        m_hasMoreSearch=false;
        for(const auto &kind:{"tracks","albums","playlists"}) {
            const auto group=page.value(kind).toObject();
            m_searchResults+=catalogItems(group.value("items").toArray());
            m_hasMoreSearch=m_hasMoreSearch||!group.value("next").toString().isEmpty();
        }
        m_searchOffset+=10;m_searchLoading=false;
        emit searchChanged();emit collectionsChanged();
    },false,[this,generation]{
        if(generation!=m_searchGeneration)return;
        m_searchLoading=false;emit collectionsChanged();
    },[this,generation]{return generation==m_searchGeneration;});
}
void SpotifyClient::refreshQueue() {
    if(m_queueLoading)return;
    m_queueLoading=true;emit collectionsChanged();
    api("GET","me/player/queue",{},[this](const QJsonObject &page,int){
        m_queueItems=catalogItems(page.value("queue").toArray());
        m_queueLoading=false;emit queueChanged();emit collectionsChanged();
    },false,[this]{m_queueLoading=false;emit collectionsChanged();});
}
void SpotifyClient::loadLibrary(const QString &kind,bool more) {
    if(more) {
        if(kind==m_libraryView)loadMoreLibrary();
        return;
    }
    if(kind!="tracks"&&kind!="albums"&&kind!="playlists")return;
    ++m_libraryGeneration;m_libraryView=kind;m_libraryPath="me/"+kind;
    m_libraryItems.clear();m_libraryOffset=0;m_hasMoreLibrary=false;
    emit libraryChanged();requestLibrary(false);
}
void SpotifyClient::openCollection(const QString &uri) {
    if(!playbackUri(uri,true))return;
    ++m_libraryGeneration;m_libraryView=uri.section(':',1,1);
    const auto id=uri.section(':',2,2);
    m_libraryPath=m_libraryView=="album"?"albums/"+id+"/tracks":"playlists/"+id+"/items";
    m_libraryItems.clear();m_libraryOffset=0;m_hasMoreLibrary=false;
    emit libraryChanged();requestLibrary(false);
}
void SpotifyClient::loadMoreLibrary() {
    if(!m_hasMoreLibrary||m_libraryLoading)return;
    requestLibrary(true);
}
void SpotifyClient::requestLibrary(bool more) {
    if(m_libraryPath.isEmpty())return;
    const int generation=++m_libraryGeneration;
    m_libraryLoading=true;emit collectionsChanged();
    const QString path=m_libraryPath+"?limit=50&offset="+QString::number(m_libraryOffset);
    api("GET",path,{},[this,generation,more](const QJsonObject &page,int){
        if(generation!=m_libraryGeneration)return;
        const auto items=page.value("items").toArray();
        if(!more)m_libraryItems.clear();
        m_libraryItems+=catalogItems(items);m_libraryOffset+=items.size();
        m_hasMoreLibrary=!page.value("next").toString().isEmpty()&&!items.isEmpty();
        m_libraryLoading=false;emit libraryChanged();emit collectionsChanged();
    },false,[this,generation]{
        if(generation!=m_libraryGeneration)return;
        m_libraryLoading=false;emit collectionsChanged();
    },[this,generation]{return generation==m_libraryGeneration;});
}
void SpotifyClient::playItem(const QString &uri,bool context) {
    if(!playbackUri(uri,context))return;
    control("PUT","me/player/play",context?QJsonObject{{"context_uri",uri}}:QJsonObject{{"uris",QJsonArray{uri}}});
}
void SpotifyClient::addToQueue(const QString &uri) {
    if(!playbackUri(uri,false)||!playbackCommandAllowed())return;
    api("POST",playbackPath("me/player/queue?uri="+QString::fromLatin1(QUrl::toPercentEncoding(uri))),{},
        [this](const QJsonObject &,int){const int generation=m_generation;QTimer::singleShot(450,this,[this,generation]{if(generation==m_generation)refreshQueue();});},true);
}
