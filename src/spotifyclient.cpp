// SPDX-License-Identifier: GPL-3.0-or-later
#include "spotifyclient.h"
#include "protocol.h"
#include "spotifyconfig.h"
#include "oauthcallbackpage.h"
#ifdef Q_OS_WIN
#include "tokenstore.h"
#endif
#include <QDateTime>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QUrlQuery>
#include <algorithm>
#include <memory>
#include <limits>

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
    :QObject(parent),m_network(transport ? transport : new QNetworkAccessManager(this)),m_settings(QSettings::defaultFormat(),QSettings::UserScope,"Phosrack","Phosrack"),
      m_defaultClientId(QString::fromLatin1(Phosrack::BuildConfig::SpotifyClientId)) {
    m_clientId=m_defaultClientId;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")) {
        const auto saved=m_settings.value("spotify/clientId").toString().trimmed();
        if(!saved.isEmpty())m_clientId=saved;
        m_integratedPlayback=m_settings.value("spotify/integratedPlayback",true).toBool();
        m_streamingAuthorized=m_settings.value(streamingGrantKey(),false).toBool();
        m_signedOut=m_settings.value("spotify/signedOut/"+m_clientId,false).toBool();
    }
    loadResumeTrack();
    connect(this,&SpotifyClient::trackChanged,this,&SpotifyClient::refreshFavorite);
    connect(this,&SpotifyClient::sessionChanged,this,&SpotifyClient::favoriteChanged);
    m_track=Phosrack::playbackState({});
    m_status=tr("Not connected");
    restoreRateLimit();
    m_poll.setInterval(15000);
    connect(&m_poll,&QTimer::timeout,this,&SpotifyClient::pollPlaybackState);
    m_refresh.setSingleShot(true);
    connect(&m_refresh,&QTimer::timeout,this,&SpotifyClient::refreshToken);
    m_loginTimeout.setSingleShot(true);
    connect(&m_loginTimeout,&QTimer::timeout,this,[this]{
        m_callback.close();m_oauthState.clear();m_verifier.clear();m_connecting=false;
        m_status=tr("Not connected");setAuthorizationRequired(true);fail(tr("Sign-in timed out. Please connect again."));emit sessionChanged();
    });
    connect(&m_callback,&QTcpServer::newConnection,this,&SpotifyClient::acceptCallback);
    m_clock.setInterval(500);
    connect(&m_clock,&QTimer::timeout,this,[this]{
        updateRateLimit();
        const auto savedAt=m_resumeTrack.value("savedAt").toLongLong();
        if(savedAt&&QDateTime::currentMSecsSinceEpoch()-savedAt>=24*60*60*1000) {
            clearResumeTrack();emit trackChanged();
        } else if(track().value("playing").toBool())emit trackChanged();
    });
    m_clock.start();
}
SpotifyClient::~SpotifyClient() {
    if(authorized()&&m_resumeObserved)rememberPlayback((!m_integratedPlayback||m_localPlaybackActive)&&!track().value("uri").toString().isEmpty()?track():m_resumeTrack,true);
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
    clearSession(false,false);m_clientId=effective;saveClientIdOverride();
    restoreRateLimit();
    m_streamingAuthorized=!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&m_settings.value(streamingGrantKey(),false).toBool();
    m_signedOut=!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&m_settings.value("spotify/signedOut/"+m_clientId,false).toBool();
    loadResumeTrack();
    emit clientIdChanged();emit trackChanged();
}
void SpotifyClient::resetToDefaultClientId(){setClientId({});}
void SpotifyClient::saveClientIdOverride() {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    if(m_clientId==m_defaultClientId)m_settings.remove("spotify/clientId");
    else m_settings.setValue("spotify/clientId",m_clientId);
}
qint64 SpotifyClient::retryAfterSeconds() const {
    const qint64 remaining=m_backoffUntil-QDateTime::currentMSecsSinceEpoch();
    return remaining>0?1+(remaining-1)/1000:0;
}
QString SpotifyClient::rateLimitMessage() const {
    const auto seconds=retryAfterSeconds();
    if(seconds<=0)return {};
    const auto duration=QStringLiteral("%1:%2:%3").arg(seconds/3600,2,10,QLatin1Char('0'))
        .arg((seconds/60)%60,2,10,QLatin1Char('0')).arg(seconds%60,2,10,QLatin1Char('0'));
    const auto deadline=QLocale().toString(retryAt(),QLocale::ShortFormat);
    return (m_quotaExceeded
        ?tr("This Spotify developer account has reached its API quota. Try again in %1 (at %2).")
        :tr("Spotify has paused requests. Try again in %1 (at %2).")).arg(duration,deadline);
}
void SpotifyClient::restoreRateLimit() {
    m_backoffUntil=0;m_quotaExceeded=false;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&!m_clientId.isEmpty()) {
        const auto key="spotify/cooldown/"+m_clientId+"/";
        m_backoffUntil=m_settings.value(key+"until",0).toLongLong();
        m_quotaExceeded=m_settings.value(key+"quotaExceeded",false).toBool();
    }
    m_rateLimitPublished=rateLimited();
    emit rateLimitChanged();emit errorChanged();updatePolling();
}
void SpotifyClient::recordRateLimit(const QString &clientId,const QByteArray &retryAfter,const QJsonObject &response) {
    const qint64 now=QDateTime::currentMSecsSinceEpoch();
    bool ok=false;
    qint64 seconds=retryAfter.trimmed().toLongLong(&ok);
    qint64 deadline=0;
    // Retry-After normally contains seconds; HTTP also permits an HTTP date.
    if(ok&&seconds>=0) {
        const auto maxSeconds=(std::numeric_limits<qint64>::max()-now)/1000;
        deadline=now+std::clamp(seconds,qint64(1),maxSeconds)*1000;
    } else {
        const auto date=QDateTime::fromString(QString::fromLatin1(retryAfter),Qt::RFC2822Date);
        deadline=date.isValid()&&date.toMSecsSinceEpoch()>now?date.toMSecsSinceEpoch():now+30000;
    }
    bool quota=response.value("error").toObject().value("reason").toString()=="QUOTA_EXCEEDED";
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&!clientId.isEmpty()) {
        const auto key="spotify/cooldown/"+clientId+"/";
        m_settings.sync();
        const auto saved=m_settings.value(key+"until",0).toLongLong();
        if(saved>now)quota=quota||m_settings.value(key+"quotaExceeded",false).toBool();
        deadline=std::max(deadline,saved);
        m_settings.setValue(key+"until",deadline);
        m_settings.setValue(key+"quotaExceeded",quota);m_settings.sync();
    }
    // A reply from an old session still contributes to its application's limit.
    if(clientId!=m_clientId)return;
    if(m_backoffUntil>now)quota=quota||m_quotaExceeded;
    m_backoffUntil=std::max(m_backoffUntil,deadline);m_quotaExceeded=quota;
    m_rateLimitPublished=true;updatePolling();
    emit rateLimitChanged();emit errorChanged();
}
void SpotifyClient::updateRateLimit() {
    if(!m_rateLimitPublished)return;
    if(!rateLimited()) {
        m_rateLimitPublished=false;m_quotaExceeded=false;
        updatePolling();
        if(authorized()){pollPlaybackState();refreshFavorite();}
        else if(!m_refreshToken.isEmpty())refreshToken();
    }
    emit rateLimitChanged();emit errorChanged();
}
void SpotifyClient::deferTokenRefresh() {
    const qint64 delay=std::max(qint64(1),m_backoffUntil-QDateTime::currentMSecsSinceEpoch());
    m_refresh.start(int(std::min(delay,qint64(std::numeric_limits<int>::max()))));
    m_connecting=false;emit sessionChanged();
}
void SpotifyClient::updatePolling() {
    const bool sdkOwnsPlayback=localPlaybackReady()&&m_localPlaybackActive;
    const int interval=!m_active?60000:(m_integratedPlayback?30000:15000);
    if(m_poll.interval()!=interval)m_poll.setInterval(interval);
    if(authorized()&&!rateLimited()&&!sdkOwnsPlayback) {
        if(!m_poll.isActive())m_poll.start();
    } else m_poll.stop();
}
void SpotifyClient::pollPlaybackState() {
    if(localPlaybackReady()&&m_localPlaybackActive)return;
    refreshState();
}
void SpotifyClient::setAuthorizationRequired(bool required) {
    if(m_authorizationRequired==required)return;
    m_authorizationRequired=required;emit authorizationRequiredChanged();
}
void SpotifyClient::setSignedOut(bool signedOut) {
    m_signedOut=signedOut;
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")||m_clientId.isEmpty())return;
    m_settings.setValue("spotify/signedOut/"+m_clientId,signedOut);m_settings.sync();
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
    updatePolling();
    emit integratedPlaybackChanged();emit trackChanged();
}
QVariantMap SpotifyClient::resumeTrack() const {
    if(authorized()&&m_integratedPlayback&&!m_track.value("title").toString().isEmpty())return m_track;
    return m_resumeTrack;
}
void SpotifyClient::loadResumeTrack() {
    m_libraryWriteAuthorized=false;
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")||m_clientId.isEmpty())return;
    m_libraryWriteAuthorized=m_settings.value("spotify/libraryWriteGrant/"+m_clientId,false).toBool();
    if(m_signedOut)return;
    const auto saved=m_settings.value("spotify/resume/"+m_clientId).toMap();
    const auto age=QDateTime::currentMSecsSinceEpoch()-saved.value("savedAt").toLongLong();
    if(age>=0&&age<24*60*60*1000&&playbackUri(saved.value("uri").toString(),false))
        m_resumeTrack=saved;
    else if(!saved.isEmpty())m_settings.remove("spotify/resume/"+m_clientId);
}
void SpotifyClient::rememberPlayback(const QVariantMap &state,bool force) {
    const auto uri=state.value("uri").toString();
    const auto title=state.value("title").toString();
    if(!authorized()||!playbackUri(uri,false)||title.isEmpty()||m_signedOut)return;
    const auto now=QDateTime::currentMSecsSinceEpoch();
    const bool changedTrack=uri!=m_resumeTrack.value("uri").toString();
    const int duration=std::max(0,state.value("duration").toInt());
    const auto context=state.value("contextUri").toString();
    m_resumeTrack={{"uri",uri},{"title",title},{"artist",state.value("artist")},
        {"contextUri",playbackUri(context,true)?context:QString()},
        {"url","https://open.spotify.com/"+uri.section(':',1,1)+"/"+uri.section(':',2,2)},
        {"duration",duration},{"position",std::clamp(state.value("position").toInt(),0,duration)},
        {"savedAt",now},{"savedResume",true}};
    m_resumeObserved=true;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&!m_clientId.isEmpty()
        &&(force||changedTrack||now-m_resumeSavedAt>=5000)) {
        m_settings.setValue("spotify/resume/"+m_clientId,m_resumeTrack);
        m_resumeSavedAt=now;
    }
}
void SpotifyClient::clearResumeTrack() {
    m_resumeTrack.clear();m_resumeSavedAt=0;m_resumeObserved=false;
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")&&!m_clientId.isEmpty())
        m_settings.remove("spotify/resume/"+m_clientId);
}
QString SpotifyClient::favoriteTrackUri() const {
    auto uri=track().value("uri").toString();
    if(uri.isEmpty())uri=resumeTrack().value("uri").toString();
    return uri.startsWith("spotify:track:")&&playbackUri(uri,false)?uri:QString();
}
void SpotifyClient::refreshFavorite() {
    const auto uri=authorized()?favoriteTrackUri():QString();
    if(uri==m_favoriteUri)return;
    m_favoriteUri=uri;
    m_favoriteSaved=m_favoriteKnown=m_favoriteLoading=false;
    const int generation=++m_favoriteGeneration;
    if(uri.isEmpty()||rateLimited()||m_refreshing){emit favoriteChanged();return;}
    m_favoriteLoading=true;emit favoriteChanged();
    const auto current=[this,generation]{return generation==m_favoriteGeneration;};
    const auto failed=[this,current]{if(current()){m_favoriteLoading=false;emit favoriteChanged();}};
    apiDocument("GET","me/library/contains?uris="+QString::fromLatin1(QUrl::toPercentEncoding(uri)),{},
        [this](const QJsonDocument &result,int){
            const auto values=result.array();
            m_favoriteKnown=!values.isEmpty()&&values.at(0).isBool();
            m_favoriteSaved=m_favoriteKnown&&values.at(0).toBool();
            m_favoriteLoading=false;emit favoriteChanged();
        },false,failed,current);
}
void SpotifyClient::toggleFavorite() {
    if(!favoriteAvailable()||rateLimited()||busy())return;
    if(!m_libraryWriteAuthorized){fail(tr("Disconnect and reconnect Spotify to grant access to change liked songs."));return;}
    const auto uri=favoriteTrackUri();
    const int generation=m_favoriteGeneration;
    const bool save=!(m_favoriteKnown&&m_favoriteSaved);
    const auto current=[this,generation]{return generation==m_favoriteGeneration;};
    m_favoriteLoading=true;emit favoriteChanged();
    api(save?"PUT":"DELETE","me/library?uris="+QString::fromLatin1(QUrl::toPercentEncoding(uri)),{},
        [this,save](const QJsonObject &,int){
            m_favoriteKnown=true;m_favoriteSaved=save;m_favoriteLoading=false;emit favoriteChanged();
        },true,[this,current]{if(current()){m_favoriteLoading=false;emit favoriteChanged();}},current);
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
    updatePolling();
    emit localPlaybackReadyChanged();emit trackChanged();
}
void SpotifyClient::clearLocalPlaybackDevice() {
    const bool hadDevice=!m_localDeviceId.isEmpty();
    m_localDeviceId.clear();m_localTrack.clear();m_localPlaybackActive=false;setSdkQueue({});
    updatePolling();
    if(hadDevice)emit localPlaybackReadyChanged();
    emit trackChanged();
}
void SpotifyClient::updateLocalPlaybackState(const QVariantMap &state) {
    if(!localPlaybackReady()||!authorized())return;
    if(state.contains("volume"))m_localVolume=std::clamp(state.value("volume").toInt(),0,100);
    if(!state.value("active",true).toBool()) {
        m_localPlaybackActive=false;setSdkQueue({});updatePolling();emit trackChanged();return;
    }
    m_localTrack=state;
    m_localTrack.remove("nextTracks");
    const bool remoteActive=!m_track.value("deviceId").toString().isEmpty()&&m_track.value("deviceId").toString()!=m_localDeviceId;
    if(!remoteActive||state.value("playing").toBool()||m_localPlaybackActive)m_localPlaybackActive=true;
    if(m_localPlaybackActive){rememberPlayback(m_localTrack);setSdkQueue(state.value("nextTracks").toList());}
    m_localPositionClock.restart();updatePolling();emit trackChanged();
}
void SpotifyClient::setActive(bool value) {
    if(m_active==value)return;
    m_active=value;updatePolling();emit activeChanged();
    if(value&&authorized())pollPlaybackState();
}
void SpotifyClient::fail(const QString &message) {m_error=message;emit errorChanged();}
void SpotifyClient::clearError() {m_error.clear();emit errorChanged();}
void SpotifyClient::connectSpotify() {
    startSession(true);
}
void SpotifyClient::restoreSession() {
    if(m_restoreAttempted||m_connecting||authorized())return;
    m_restoreAttempted=true;startSession(false);
}
void SpotifyClient::continueAuthorization() {
    if(m_interactiveLogin&&!rateLimited()){beginOAuth();return;}
    m_connecting=false;m_status=tr("Not connected");setAuthorizationRequired(true);emit sessionChanged();
}
void SpotifyClient::startSession(bool interactive) {
    if(m_connecting||authorized())return;
    m_interactiveLogin=interactive;
    if(!QRegularExpression("^[a-fA-F0-9]{32}$").match(m_clientId).hasMatch()) {
        setAuthorizationRequired(true);
        if(interactive)fail(tr("Enter the 32-character Client ID of your Spotify application."));
        return;
    }
    clearError();m_connecting=true;setAuthorizationRequired(false);
    // A deliberate sign-out is authoritative even if protected storage was
    // locked or deletion failed. Only a fresh browser grant can undo it.
    if(m_signedOut&&!interactive){continueAuthorization();return;}
    if(m_reauthorizeStreaming){continueAuthorization();return;}
    // Tests never prompt the real desktop wallet. Explicit OAuth tests install
    // a QDesktopServices URL handler to capture consent without a browser.
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")) {
        if(m_signedOut||m_refreshToken.isEmpty())continueAuthorization();else refreshToken();
        return;
    }
#ifdef Q_OS_WIN
    m_status=tr("Opening Windows Credential Manager…");emit sessionChanged();
    const int generation=m_generation;
    QTimer::singleShot(0,this,[this,generation]{
        if(generation!=m_generation)return;
        if(m_signedOut){continueAuthorization();return;}
        const auto saved=Phosrack::NativeTokenStore::readRefreshToken(m_clientId);
        if(saved.status==Phosrack::NativeTokenStore::ReadStatus::Found) {
            m_refreshToken=saved.token;m_status=tr("Restoring session…");emit sessionChanged();refreshToken();
        } else {
            if(saved.status==Phosrack::NativeTokenStore::ReadStatus::Unavailable)
                fail(tr("Windows Credential Manager is unavailable. This session will only be kept in memory."));
            continueAuthorization();
        }
    });
#else
    m_status=tr("Opening KWallet…");emit sessionChanged();
    if(m_wallet&&m_wallet->isOpen()) {withWallet();return;}
    if(m_wallet)delete m_wallet;
    m_wallet=KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),0,KWallet::Wallet::Asynchronous);
    if(!m_wallet) {fail(tr("KWallet is unavailable. This session will only be kept in memory."));continueAuthorization();return;}
    const int generation=m_generation;
    connect(m_wallet,&KWallet::Wallet::walletOpened,this,[this,generation](bool open){
        if(generation!=m_generation)return;
        if(open)withWallet();else {fail(tr("KWallet was not opened. This session will only be kept in memory."));continueAuthorization();}
    });
#endif
}
#ifndef Q_OS_WIN
void SpotifyClient::withWallet() {
    if(!m_wallet->hasFolder("Phosrack"))m_wallet->createFolder("Phosrack");
    if(!m_wallet->setFolder("Phosrack")) {continueAuthorization();return;}
    if(m_signedOut){continueAuthorization();return;}
    QString token;
    if(m_wallet->readPassword("spotify:"+m_clientId,token)==0&&!token.isEmpty()) {
        m_refreshToken=token.toUtf8();m_status=tr("Restoring session…");emit sessionChanged();refreshToken();
    } else continueAuthorization();
}
#endif
void SpotifyClient::beginOAuth() {
    m_callback.close();
    if(!m_callback.listen(QHostAddress::LocalHost,43821)) {
        m_connecting=false;m_status=tr("Not connected");
        setAuthorizationRequired(true);
        fail(tr("Cannot open 127.0.0.1:43821. Close another Phosrack sign-in and try again."));emit sessionChanged();return;
    }
    m_verifier=Phosrack::randomToken();m_oauthState=Phosrack::randomToken();
    QUrl url("https://accounts.spotify.com/authorize");
    QUrlQuery query;
    query.addQueryItem("response_type","code");query.addQueryItem("client_id",m_clientId);
    query.addQueryItem("redirect_uri",redirectUri);query.addQueryItem("state",QString::fromLatin1(m_oauthState));
    query.addQueryItem("code_challenge_method","S256");
    query.addQueryItem("code_challenge",QString::fromLatin1(Phosrack::pkceChallenge(m_verifier)));
    const QString scopes=QStringLiteral("user-read-playback-state user-modify-playback-state user-read-currently-playing user-library-read user-library-modify playlist-read-private playlist-read-collaborative");
    m_oauthStreamingRequested=m_integratedPlayback;
    query.addQueryItem("scope",m_oauthStreamingRequested?QStringLiteral("streaming user-read-private user-read-email ")+scopes:scopes);
    url.setQuery(query);m_status=tr("Authorize Phosrack in your browser…");emit sessionChanged();m_loginTimeout.start(180000);
    if(!QDesktopServices::openUrl(url)) {
        m_callback.close();m_loginTimeout.stop();m_connecting=false;m_status=tr("Not connected");
        setAuthorizationRequired(true);
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
            const auto page=valid?(result.error.isEmpty()?Phosrack::OAuthPageState::AuthorizationReceived:Phosrack::OAuthPageState::Denied)
                :(generation!=m_generation||m_oauthState.isEmpty()?Phosrack::OAuthPageState::Expired:Phosrack::OAuthPageState::InvalidRequest);
            socket->write(Phosrack::oauthCallbackResponse(page));
            socket->disconnectFromHost();
            if(!valid)return;
            m_callback.close();m_loginTimeout.stop();m_oauthState.clear();
            if(!result.error.isEmpty()) {
                m_verifier.clear();m_connecting=false;m_status=tr("Not connected");
                setAuthorizationRequired(true);
                fail(tr("Spotify authorization was cancelled or denied."));emit sessionChanged();return;
            }
            const auto body=formBody({{"grant_type","authorization_code"},{"code",result.code},
                {"redirect_uri",redirectUri},{"client_id",m_clientId},{"code_verifier",QString::fromLatin1(m_verifier)}});
            m_verifier.clear();requestToken(body);
        });
    }
}
bool SpotifyClient::persistRefresh() {
    if(m_refreshToken.isEmpty())return false;
#ifdef Q_OS_WIN
    if(!Phosrack::NativeTokenStore::writeRefreshToken(m_clientId,m_refreshToken)) {
        fail(tr("Could not save the session in Windows Credential Manager. It remains available until Phosrack closes."));
        return false;
    }
    return true;
#else
    if(m_wallet&&m_wallet->isOpen()&&!m_refreshToken.isEmpty()) {
        if(!m_wallet->setFolder("Phosrack")||m_wallet->writePassword("spotify:"+m_clientId,QString::fromUtf8(m_refreshToken))!=0) {
            fail(tr("Could not save the session in KWallet. It remains available until Phosrack closes."));
            return false;
        }
        return true;
    }
    return false;
#endif
}
bool SpotifyClient::forgetRefresh() {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return true;
#ifdef Q_OS_WIN
    return Phosrack::NativeTokenStore::removeRefreshToken(m_clientId);
#else
    if(m_wallet&&m_wallet->isOpen()) {
        if(!m_wallet->setFolder("Phosrack"))return false;
        return m_wallet->removeEntry("spotify:"+m_clientId)==0;
    }
    return false;
#endif
}
void SpotifyClient::refreshToken() {
    if(m_refreshing||m_refreshToken.isEmpty()||m_signedOut)return;
    if(rateLimited()){deferTokenRefresh();return;}
    requestToken(formBody({{"grant_type","refresh_token"},{"refresh_token",QString::fromUtf8(m_refreshToken)},{"client_id",m_clientId}}));
}
void SpotifyClient::requestToken(const QByteArray &form) {
    if(rateLimited()) {
        if(!m_refreshToken.isEmpty())deferTokenRefresh();
        else {m_connecting=false;setAuthorizationRequired(true);emit sessionChanged();}
        return;
    }
    m_refreshing=true;
    QNetworkRequest request(QUrl("https://accounts.spotify.com/api/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");request.setTransferTimeout(15000);
    auto *reply=m_network->post(request,form);
    const int generation=m_generation;
    const auto applicationId=m_clientId;
    const bool newGrant=QUrlQuery(QString::fromLatin1(form)).queryItemValue("grant_type")=="authorization_code";
    const bool requestedStreamingGrant=newGrant&&m_oauthStreamingRequested;
    connect(reply,&QNetworkReply::finished,this,[this,reply,generation,applicationId,newGrant,requestedStreamingGrant]{
        reply->deleteLater();
        const int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto data=QJsonDocument::fromJson(reply->readAll()).object();
        if(code==429)recordRateLimit(applicationId,reply->rawHeader("Retry-After"),data);
        if(generation!=m_generation)return;
        m_refreshing=false;m_connecting=false;
        if(code==429) {
            if(!m_refreshToken.isEmpty())deferTokenRefresh();else setAuthorizationRequired(true);
            emit sessionChanged();return;
        }
        if(code!=200||data.value("access_token").toString().isEmpty()) {
            if(data.value("error").toString()=="invalid_grant") {
                setSignedOut(true);
                clearResumeTrack();
                forgetRefresh();
                clearSession(false);
                m_status=tr("Please reconnect");fail(tr("Authorization expired or was revoked. Connect Spotify again."));
            } else {
                const bool permanent=code>=400&&code<500&&code!=408;
                if(permanent)clearSession(false);
                m_status=authorized()?tr("Cannot reach Spotify"):tr("Not connected");
                fail(tr("Could not obtain a Spotify session. Check the connection, Client ID and redirect URI."));
                if(!permanent&&!m_refreshToken.isEmpty())m_refresh.start(30000);
                else if(!authorized())setAuthorizationRequired(true);
            }
            emit sessionChanged();return;
        }
        const bool streamingGrant=data.contains("scope")?hasStreamingGrant(data.value("scope").toString()):(newGrant?requestedStreamingGrant:m_streamingAuthorized);
        m_streamingAuthorized=streamingGrant;
        if(data.contains("scope"))m_libraryWriteAuthorized=data.value("scope").toString().split(' ').contains("user-library-modify");
        else if(newGrant)m_libraryWriteAuthorized=true;
        if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))
            m_settings.setValue("spotify/libraryWriteGrant/"+m_clientId,m_libraryWriteAuthorized);
        if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))m_settings.setValue(streamingGrantKey(),streamingGrant);
        if(m_integratedPlayback&&!streamingGrant){requireStreamingAuthorization();return;}
        m_accessToken=data.value("access_token").toString().toUtf8();
        setAuthorizationRequired(false);
        m_reauthorizeStreaming=false;
        const auto renewed=data.value("refresh_token").toString();
        if(!renewed.isEmpty()||newGrant)m_refreshToken=renewed.toUtf8();
        if(newGrant)setSignedOut(true);
        const bool saved=persistRefresh();
        if(newGrant) {
            // Do not re-enable automatic restoration of an older credential if
            // saving the new grant failed. The current memory session can run.
            if(saved)setSignedOut(false);else m_signedOut=false;
        }
        m_status=tr("Connected to Spotify");
        m_refresh.start(std::max(30,data.value("expires_in").toInt(3600)-60)*1000);
        updatePolling();emit accessTokenChanged();emit sessionChanged();
        // Token renewal does not invalidate already fresh playback/device data.
        if(!m_positionClock.isValid())refreshState();
        refreshDevices();
    });
}
void SpotifyClient::disconnectSpotify() {
    // Persist intent before attempting deletion; neither a locked wallet nor
    // a process exit should cause an unwanted automatic reconnection.
    setSignedOut(true);
    clearResumeTrack();
    clearSession(true,false);
}
void SpotifyClient::suspendSession() {
    clearSession(false,false);
}
void SpotifyClient::clearSession(bool forgetStoredToken,bool requestAuthorization) {
    m_resumeTrack.clear();m_resumeSavedAt=0;m_resumeObserved=false;
    ++m_generation;m_callback.close();m_loginTimeout.stop();m_poll.stop();m_refresh.stop();
    const bool removed=!forgetStoredToken||forgetRefresh();
    m_accessToken.clear();m_refreshToken.clear();m_oauthState.clear();m_verifier.clear();
    m_favoriteUri.clear();m_favoriteKnown=m_favoriteSaved=m_favoriteLoading=false;++m_favoriteGeneration;
    m_reauthorizeStreaming=false;m_oauthStreamingRequested=false;clearLocalPlaybackDevice();emit accessTokenChanged();
    m_connecting=m_refreshing=m_polling=m_command=false;m_interactiveLogin=false;
    m_devicesLoading=false;m_devicesFetchedAt=0;m_positionClock.invalidate();
    setAuthorizationRequired(requestAuthorization);
    m_track=Phosrack::playbackState({});m_devices.clear();clearCollections();m_status=tr("Not connected");clearError();
    if(!removed) {
#ifdef Q_OS_WIN
        fail(tr("The saved session could not be removed from Windows Credential Manager. Automatic sign-in is disabled; you can remove the Phosrack entry in Credential Manager."));
#else
        fail(tr("The saved session could not be removed from KWallet. Automatic sign-in is disabled; you can remove the Phosrack entry in KWallet Manager."));
#endif
    }
    emit sessionChanged();emit trackChanged();emit devicesChanged();
}
void SpotifyClient::api(const QByteArray &method,const QString &path,const QJsonObject &body,Result done,bool command,std::function<void()> failed,std::function<bool()> current) {
    apiDocument(method,path,body,[done](const QJsonDocument &data,int code){if(done)done(data.object(),code);},command,failed,current);
}
void SpotifyClient::apiDocument(const QByteArray &method,const QString &path,const QJsonObject &body,DocumentResult done,bool command,std::function<void()> failed,std::function<bool()> current) {
    if(!authorized()||m_refreshing) {if(command)fail(tr("Connect Spotify or wait for the session to refresh."));if(failed)failed();return;}
    if(rateLimited()) {if(command)emit errorChanged();if(failed)failed();return;}
    if(command&&m_command){if(failed)failed();return;}
    if(command){m_command=true;emit sessionChanged();}
    QNetworkRequest request(QUrl("https://api.spotify.com/v1/"+path));
    request.setRawHeader("Authorization","Bearer "+m_accessToken);request.setTransferTimeout(12000);
    if(!body.isEmpty())request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    auto *reply=m_network->sendCustomRequest(request,method,body.isEmpty()?QByteArray():QJsonDocument(body).toJson(QJsonDocument::Compact));
    const int generation=m_generation;
    const auto applicationId=m_clientId;
    connect(reply,&QNetworkReply::finished,this,[this,reply,generation,applicationId,command,done,path,method,failed,current]{
        reply->deleteLater();
        const int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto data=QJsonDocument::fromJson(reply->readAll());
        // Even an obsolete search, or a reply received after Disconnect, must
        // not lose the server's cooldown for the application that sent it.
        if(code==429)recordRateLimit(applicationId,reply->rawHeader("Retry-After"),data.object());
        if(generation!=m_generation)return;
        if(path=="me/player" && method=="GET")m_polling=false;
        if(command){m_command=false;emit sessionChanged();}
        if(current&&!current())return;
        if(code==429) {if(failed)failed();return;}
        if(code==401) {refreshToken();if(command)fail(tr("Refreshing session; try the control again when it finishes."));if(failed)failed();return;}
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
        if(!m_integratedPlayback||!m_localPlaybackActive)rememberPlayback(m_track);
        if(localPlaybackReady()) {
            m_localPlaybackActive=m_track.value("deviceId").toString()==m_localDeviceId;
            if(m_localPlaybackActive) {
                m_localTrack=m_track;
                m_localTrack["canVolume"]=true;
                m_localPositionClock.restart();
            }
        }
        updatePolling();emit trackChanged();
    });
}
void SpotifyClient::refreshDevices() {
    if(m_devicesLoading||!authorized()||m_refreshing||rateLimited())return;
    if(m_devicesFetchedAt>0&&QDateTime::currentMSecsSinceEpoch()-m_devicesFetchedAt<60000)return;
    m_devicesLoading=true;
    api("GET","me/player/devices",{},[this](const QJsonObject &o,int){
        m_devices.clear();for(const auto &value:o.value("devices").toArray()) {
            const auto d=value.toObject();if(d.value("id").toString().isEmpty())continue;
            m_devices<<QVariantMap{{"id",d.value("id").toString()},{"name",d.value("name").toString()},
                {"active",d.value("is_active").toBool()},{"restricted",d.value("is_restricted").toBool()}};
        }
        m_devicesLoading=false;m_devicesFetchedAt=QDateTime::currentMSecsSinceEpoch();emit devicesChanged();
    },false,[this]{m_devicesLoading=false;});
}
void SpotifyClient::control(const QByteArray &method,const QString &path,const QJsonObject &body) {
    if(!playbackCommandAllowed())return;
    if(m_integratedPlayback&&path=="me/player/play")emit localPlaybackCommand("activate",{});
    api(method,playbackPath(path),body,[this](const QJsonObject &,int){const int g=m_generation;QTimer::singleShot(450,this,[this,g]{if(g==m_generation)pollPlaybackState();});},true);
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
            const auto saved=resumeTrack();
            const auto uri=saved.value("uri").toString();
            if(playbackUri(uri,false)) {
                const int duration=saved.value("duration").toInt();
                const int position=saved.value("position").toInt();
                QJsonObject body{{"position_ms",position>=duration?0:position}};
                const auto context=saved.value("contextUri").toString();
                if(playbackUri(context,true)) {
                    body["context_uri"]=context;
                    body["offset"]=QJsonObject{{"uri",uri}};
                } else body["uris"]=QJsonArray{uri};
                control("PUT","me/player/play",body);
            } else fail(tr("Choose a track from Search or Library to start playback in Phosrack."));
            return;
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
    m_searchResults.clear();m_queueItems.clear();m_libraryItems.clear();m_queueForcePending=false;
    m_searchQuery.clear();m_libraryView.clear();m_libraryPath.clear();
    m_queueFetchedAt=0;m_queueTrackUri.clear();
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
QVariantList SpotifyClient::queueItems() const {
    if(!m_sdkQueueItems.isEmpty()&&(m_queueItems.isEmpty()||m_queueTrackUri!=queueTrackIdentity()))
        return m_sdkQueueItems;
    return m_queueItems;
}
void SpotifyClient::setSdkQueue(const QVariantList &items) {
    if(items==m_sdkQueueItems)return;
    m_sdkQueueItems=items;emit queueChanged();
}
QString SpotifyClient::queueTrackIdentity() const {
    const auto current=track();
    auto uri=current.value("uri").toString();
    if(uri.isEmpty())uri=current.value("url").toString();
    if(uri.isEmpty()&&m_integratedPlayback&&!m_localPlaybackActive) {
        uri=m_track.value("uri").toString();
        if(uri.isEmpty())uri=m_track.value("url").toString();
    }
    return uri;
}
void SpotifyClient::refreshQueue(bool force) {
    if(m_queueLoading){m_queueForcePending=m_queueForcePending||force;return;}
    const auto uri=queueTrackIdentity();
    if(!authorized()||m_refreshing||rateLimited()
        ||(!force&&m_queueFetchedAt>0&&m_queueTrackUri==uri&&QDateTime::currentMSecsSinceEpoch()-m_queueFetchedAt<30000)) {
        emit collectionsChanged();return;
    }
    m_queueLoading=true;emit collectionsChanged();
    api("GET","me/player/queue",{},[this,uri](const QJsonObject &page,int){
        m_queueItems=catalogItems(page.value("queue").toArray());
        m_queueFetchedAt=QDateTime::currentMSecsSinceEpoch();m_queueTrackUri=uri;
        m_queueLoading=false;emit queueChanged();emit collectionsChanged();
        // A track change while this request was in flight must not leave the
        // previous queue cached indefinitely once periodic polling is gone.
        const bool force=m_queueForcePending;m_queueForcePending=false;
        if(force||queueTrackIdentity()!=uri)refreshQueue(force);
    },false,[this]{m_queueLoading=m_queueForcePending=false;emit collectionsChanged();});
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
        [this](const QJsonObject &,int){m_queueFetchedAt=0;const int generation=m_generation;QTimer::singleShot(450,this,[this,generation]{if(generation==m_generation)refreshQueue();});},true);
}
