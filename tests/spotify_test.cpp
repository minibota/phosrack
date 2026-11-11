// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QQueue>
#include <QUrlQuery>
#include <QTemporaryDir>
#include <QDesktopServices>
#include <cstring>
#include "spotifyclient.h"
#include "spotifyconfig.h"

class MemoryReply : public QNetworkReply {
public:
    MemoryReply(const QNetworkRequest &request,int status,const QByteArray &body,int delay,const QByteArray &retryAfter,QObject *parent)
        :QNetworkReply(parent),data(body) {
        setRequest(request);setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);
        setRawHeader("Retry-After",retryAfter);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        QTimer::singleShot(delay,this,[this]{setFinished(true);emit readyRead();emit finished();});
    }
    void abort() override {}
    qint64 bytesAvailable() const override {return data.size()-position+QNetworkReply::bytesAvailable();}
protected:
    qint64 readData(char *destination,qint64 maxSize) override {
        const qint64 count=std::min(maxSize,qint64(data.size())-position);
        if(count<=0)return -1;
        std::memcpy(destination,data.constData()+position,size_t(count));position+=count;return count;
    }
private:
    QByteArray data;qint64 position=0;
};
class MemoryNetwork : public QNetworkAccessManager {
public:
    struct Response {int status;QByteArray data;int delay=0;QByteArray retryAfter="120";};
    struct Sent {QNetworkRequest request;QByteArray method;QByteArray body;};
    QQueue<Response> responses;
    QList<Sent> requests;
protected:
    QNetworkReply *createRequest(Operation operation,const QNetworkRequest &request,QIODevice *body) override {
        const QByteArray method=operation==PostOperation?QByteArray("POST"):request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray();
        requests.append({request,method,body?body->readAll():QByteArray()});
        const auto response=responses.isEmpty()?Response{204,{}}:responses.dequeue();
        return new MemoryReply(request,response.status,response.data,response.delay,response.retryAfter,this);
    }
};
class AuthorizationUrlCapture : public QObject {
    Q_OBJECT
public:
    QUrl url;
public slots:
    void capture(const QUrl &value) {url=value;}
};
class SpotifyClientTest : public QObject {
    Q_OBJECT
    static void session(SpotifyClient &client, bool integrated=false) {
        client.m_accessToken="test-access";
        client.m_integratedPlayback=integrated;client.m_streamingAuthorized=true;
        client.m_track={{"canControl",true},{"canToggle",true},{"canVolume",true},{"canSeek",true},{"playing",true},{"duration",100000}};
    }
private slots:
    void unconfiguredApplicationDoesNotStartSignIn() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        QVERIFY(client.integratedPlayback());
        if(client.hasDefaultClientId())QSKIP("This private build supplies a default application.");
        client.connectSpotify();
        QVERIFY(!client.authorized());QVERIFY(!client.busy());
        QVERIFY(!client.m_callback.isListening());
        QVERIFY(client.error().contains("32-character Client ID"));
        QVERIFY(transport.requests.isEmpty());
    }
    void compiledDefaultAndStableAssignmentsPreserveSession() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        const auto compiled=QString::fromLatin1(Phosrack::BuildConfig::SpotifyClientId);
        QCOMPARE(client.clientId(),compiled);
        QCOMPARE(client.hasDefaultClientId(),!compiled.isEmpty());
        QCOMPARE(client.usesDefaultClientId(),!compiled.isEmpty());
        session(client);client.m_refreshToken="test-refresh";
        const auto generation=client.m_generation;
        QSignalSpy changed(&client,&SpotifyClient::clientIdChanged);
        client.setClientId(" "+compiled+" ");
        client.resetToDefaultClientId();
        QVERIFY(client.authorized());QCOMPARE(client.m_refreshToken,QByteArray("test-refresh"));
        QCOMPARE(client.m_generation,generation);QCOMPARE(changed.count(),0);
        const auto custom=compiled==QString(32,'a')?QString(32,'b'):QString(32,'a');
        client.setClientId(custom);
        QCOMPARE(client.clientId(),custom);QVERIFY(!client.usesDefaultClientId());
        QVERIFY(!client.authorized());QVERIFY(client.m_refreshToken.isEmpty());QCOMPARE(changed.count(),1);
        client.setClientId("  ");
        QCOMPARE(client.clientId(),compiled);QCOMPARE(client.usesDefaultClientId(),!compiled.isEmpty());
        QCOMPARE(changed.count(),2);QVERIFY(transport.requests.isEmpty());
    }
    void savedOverridesSurviveUpgradeAndResetWithoutLeavingDefaultPinned() {
        struct IsolatedConfiguration {
            QTemporaryDir directory;
            const QSettings::Format format=QSettings::defaultFormat();
            const bool testModeWasSet=qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE");
            const QByteArray oldTestMode=qgetenv("PHOSRACK_TEST_MODE");
            IsolatedConfiguration(){
                QSettings::setDefaultFormat(QSettings::IniFormat);
                QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());
                qunsetenv("PHOSRACK_TEST_MODE");
            }
            ~IsolatedConfiguration(){
                QSettings::setDefaultFormat(format);
                if(testModeWasSet)qputenv("PHOSRACK_TEST_MODE",oldTestMode);else qunsetenv("PHOSRACK_TEST_MODE");
            }
        } isolated;
        QVERIFY(isolated.directory.isValid());
        const auto compiled=QString::fromLatin1(Phosrack::BuildConfig::SpotifyClientId);
        const auto legacy=compiled==QString(32,'c')?QString(32,'d'):QString(32,'c');
        QSettings settings(QSettings::IniFormat,QSettings::UserScope,"Phosrack","Phosrack");
        settings.setValue("spotify/clientId",legacy);settings.sync();
        {
            SpotifyClient restored;
            QCOMPARE(restored.clientId(),legacy);QVERIFY(!restored.usesDefaultClientId());
            restored.resetToDefaultClientId();
            QCOMPARE(restored.clientId(),compiled);
            QVERIFY(!restored.m_settings.contains("spotify/clientId"));
        }
        {
            SpotifyClient normal;
            QCOMPARE(normal.clientId(),compiled);
            normal.setClientId(legacy);
            QCOMPARE(normal.m_settings.value("spotify/clientId").toString(),legacy);
        }
        {
            SpotifyClient custom;
            QCOMPARE(custom.clientId(),legacy);
        }
        // Empty legacy settings fall back; test mode ignores all saved settings
        // while keeping the application's compile-time default deterministic.
        settings.setValue("spotify/clientId","");settings.sync();
        {SpotifyClient blankLegacy;QCOMPARE(blankLegacy.clientId(),compiled);}
        settings.setValue("spotify/clientId",legacy);settings.sync();
        qputenv("PHOSRACK_TEST_MODE","1");
        SpotifyClient testClient;
        QCOMPARE(testClient.clientId(),compiled);
        testClient.setClientId(QString(32,'e'));
        QCOMPARE(settings.value("spotify/clientId").toString(),legacy);
    }
    void accessDenialExplainsDevelopmentAllowlistForPersonalApplications() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({403,"{}"});client.refreshState();QTRY_VERIFY(!client.m_polling);
        QVERIFY(client.error().contains("allowed to use this developer application"));
        QVERIFY(client.error().contains("Premium"));
        QVERIFY(!client.collectionBusy());
    }
    void playbackRequestsAreRealApiShapes() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        client.playPause();
        QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests.size(),1);
        QCOMPARE(transport.requests[0].method,QByteArray("PUT"));
        QCOMPARE(transport.requests[0].request.url().path(),QString("/v1/me/player/pause"));
        QCOMPARE(transport.requests[0].request.rawHeader("Authorization"),QByteArray("Bearer test-access"));
        client.setVolume(200);QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests[1].request.url().query(),QString("volume_percent=100"));
        client.seek(-100);QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests[2].request.url().query(),QString("position_ms=0"));
    }
    void handlesNoActiveDevice() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({204,{}});client.refreshState();
        QTRY_VERIFY(!client.m_polling);
        QVERIFY(!client.track().value("canControl").toBool());
        QVERIFY(client.authorized());
    }
    void throttlesRateLimit() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({429,"{}"});client.refreshState();QTRY_VERIFY(!client.m_polling);
        client.refreshState();client.playPause();
        QCOMPARE(transport.requests.size(),1);
        QVERIFY(client.error().contains("pause"));
    }
    void startupRestoresStoredSessionWithoutOpeningAuthorization() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_clientId=QString(32,'a');client.m_refreshToken="saved-refresh";
        client.m_streamingAuthorized=true;
        transport.responses.enqueue({200,R"({"access_token":"restored-access","expires_in":3600})"});
        client.restoreSession();QTRY_VERIFY(client.authorized());
        QVERIFY(!client.authorizationRequired());QVERIFY(!client.m_callback.isListening());
        QTRY_COMPARE(transport.requests.size(),3);
        client.restoreSession();QCOMPARE(transport.requests.size(),3);
    }
    void startupWithoutCredentialsRequestsConsentButDoesNotOpenBrowser() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        QVERIFY(!client.authorizationRequired());
        client.restoreSession();
        QVERIFY(client.authorizationRequired());QVERIFY(!client.busy());
        QVERIFY(!client.m_callback.isListening());QVERIFY(transport.requests.isEmpty());
        SpotifyClient configured(nullptr,&transport);configured.m_clientId=QString(32,'a');
        configured.restoreSession();
        QVERIFY(configured.authorizationRequired());QVERIFY(!configured.busy());
        QVERIFY(!configured.m_callback.isListening());QVERIFY(transport.requests.isEmpty());
    }
    void permanentTokenFailureRequiresUserActionWithoutRepeatedRequests() {
        for(const auto error:{"invalid_client","invalid_request","invalid_grant"}) {
            MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
            client.m_clientId=QString(32,'a');client.m_refreshToken="saved-refresh";
            transport.responses.enqueue({400,QJsonDocument(QJsonObject{{"error",error}}).toJson()});
            client.restoreSession();QTRY_VERIFY(client.authorizationRequired());
            QVERIFY(!client.m_refresh.isActive());QVERIFY(!client.m_callback.isListening());
            QVERIFY(!client.authorized());QVERIFY(!client.busy());QCOMPARE(transport.requests.size(),1);
        }
    }
    void onlyFreshAuthorizationClearsExplicitSignOut() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_clientId=QString(32,'a');client.m_refreshToken="leftover-refresh";
        client.setSignedOut(true);client.refreshToken();QVERIFY(transport.requests.isEmpty());
        transport.responses.enqueue({200,R"({"access_token":"new-access","refresh_token":"new-refresh","scope":"streaming user-read-private user-read-email"})"});
        client.requestToken("grant_type=authorization_code");QTRY_VERIFY(client.authorized());
        QVERIFY(!client.m_signedOut);QVERIFY(!client.authorizationRequired());
        QCOMPARE(client.m_refreshToken,QByteArray("new-refresh"));
    }
    void manualConnectionAfterSignOutOpensFreshConsentWithoutReusingToken() {
        AuthorizationUrlCapture capture;
        struct UrlHandler {
            explicit UrlHandler(QObject *receiver){QDesktopServices::setUrlHandler("https",receiver,"capture");}
            ~UrlHandler(){QDesktopServices::unsetUrlHandler("https");}
        } handler(&capture);
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_clientId=QString(32,'a');client.m_refreshToken="leftover-refresh";client.setSignedOut(true);
        client.connectSpotify();
        QVERIFY(client.m_callback.isListening());QVERIFY(capture.url.isValid());
        QCOMPARE(capture.url.host(),QString("accounts.spotify.com"));
        QCOMPARE(capture.url.path(),QString("/authorize"));
        QVERIFY(transport.requests.isEmpty());QVERIFY(client.m_signedOut);
        client.disconnectSpotify();QVERIFY(!client.m_callback.isListening());
    }
    void startupAndRefreshRespectExistingCooldown() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_clientId=QString(32,'a');client.m_refreshToken="saved-refresh";
        client.recordRateLimit(client.clientId(),"120",{});
        client.restoreSession();client.refreshToken();client.refreshQueue(true);client.refreshDevices();
        QVERIFY(transport.requests.isEmpty());QVERIFY(!client.busy());
        QVERIFY(!client.authorizationRequired());QVERIFY(client.m_refresh.isActive());
        QCOMPARE(client.retryAfterSeconds(),qint64(120));
        QVERIFY(client.error().contains("0:02:00"));
    }
    void obsoleteRequestsRetainLongerServerCooldownAndQuotaReason() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({429,R"({"error":{"reason":"QUOTA_EXCEEDED"}})",10,"172800"});
        transport.responses.enqueue({429,"{}",30,"60"});
        client.search("old");client.search("new");
        QTRY_VERIFY(client.rateLimited());QTRY_VERIFY(!client.collectionBusy());
        QVERIFY(client.retryAfterSeconds()>172790);
        QVERIFY(client.rateLimitMessage().contains("developer account"));
        client.refreshState();client.refreshDevices();client.refreshQueue(true);
        QCOMPARE(transport.requests.size(),2);
        const auto deadline=client.retryAt();
        client.disconnectSpotify();
        QCOMPARE(client.retryAt(),deadline);QVERIFY(client.rateLimited());
    }
    void lateRateLimitAfterDisconnectIsStillHonored() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({429,"{}",10,"26156"});
        client.refreshState();client.disconnectSpotify();
        QTRY_VERIFY(client.rateLimited());
        QVERIFY(client.retryAfterSeconds()>26150);QVERIFY(!client.authorized());
        QVERIFY(!client.authorizationRequired());QVERIFY(client.m_signedOut);
        QVERIFY(client.rateLimitMessage().contains("07:15:"));
    }
    void cooldownRestoresAcrossInstancesAndClientIdSwitches() {
        struct IsolatedConfiguration {
            QTemporaryDir directory;
            QSettings::Format format=QSettings::defaultFormat();
            QByteArray testMode=qgetenv("PHOSRACK_TEST_MODE");
            IsolatedConfiguration() {
                QSettings::setDefaultFormat(QSettings::IniFormat);
                QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());
                qunsetenv("PHOSRACK_TEST_MODE");
            }
            ~IsolatedConfiguration() {
                QSettings::setDefaultFormat(format);qputenv("PHOSRACK_TEST_MODE",testMode);
            }
        } isolated;
        QVERIFY(isolated.directory.isValid());
        const auto application=QString(32,'f');QDateTime deadline;
        {
            MemoryNetwork transport;SpotifyClient client(nullptr,&transport);client.setClientId(application);
            client.recordRateLimit(application,"172800",{});deadline=client.retryAt();
            client.clearSession(false);QCOMPARE(client.retryAt(),deadline);
        }
        {
            MemoryNetwork transport;SpotifyClient restored(nullptr,&transport);
            QCOMPARE(restored.clientId(),application);QCOMPARE(restored.retryAt(),deadline);
            session(restored);restored.refreshState();restored.refreshQueue(true);
            QVERIFY(transport.requests.isEmpty());
            restored.setClientId(QString(32,'e'));QVERIFY(!restored.rateLimited());
            restored.setClientId(application);QCOMPARE(restored.retryAt(),deadline);
            restored.recordRateLimit(application,"60",{});QCOMPARE(restored.retryAt(),deadline);
        }
        {
            // Persist the user's sign-out intent independently of protected
            // storage: this test never reads or mutates actual credentials.
            MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
            client.setSignedOut(true);QVERIFY(client.m_settings.value("spotify/signedOut/"+application).toBool());
        }
        {
            MemoryNetwork transport;SpotifyClient signedOut(nullptr,&transport);
            QVERIFY(signedOut.m_signedOut);signedOut.m_refreshToken="leftover-protected-token";
            signedOut.restoreSession();
            QVERIFY(signedOut.authorizationRequired());QVERIFY(transport.requests.isEmpty());
            QVERIFY(!signedOut.busy());QVERIFY(!signedOut.m_callback.isListening());
            signedOut.suspendSession();QVERIFY(signedOut.m_signedOut);
#ifndef Q_OS_WIN
            // A locked/unavailable KWallet cannot delete the entry. Sign-out
            // remains durable and no immediate reconnect prompt is shown.
            signedOut.disconnectSpotify();
            QVERIFY(signedOut.m_signedOut);QVERIFY(!signedOut.authorizationRequired());
            QVERIFY(signedOut.m_settings.value("spotify/signedOut/"+application).toBool());
            QVERIFY(signedOut.m_error.contains("Automatic sign-in is disabled"));
#endif
        }
    }
    void cooldownParsesHttpDatesAndExpiresWithoutStaleError() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        const auto future=QDateTime::currentDateTimeUtc().addDays(2);
        client.recordRateLimit(client.clientId(),future.toString(Qt::RFC2822Date).toLatin1(),{});
        QVERIFY(client.retryAfterSeconds()>172790);
        QSignalSpy changed(&client,&SpotifyClient::rateLimitChanged);
        client.m_backoffUntil=QDateTime::currentMSecsSinceEpoch()-1;
        client.updateRateLimit();
        QVERIFY(!client.rateLimited());QVERIFY(client.rateLimitMessage().isEmpty());
        QVERIFY(client.error().isEmpty());QCOMPARE(changed.size(),1);
    }
    void queueRefreshSharesFreshResultsAndInvalidatesOnTrackChange() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        client.m_track["uri"]="spotify:track:first";
        client.refreshQueue();client.refreshQueue();QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(transport.requests.size(),1);
        QSignalSpy completed(&client,&SpotifyClient::collectionsChanged);
        client.refreshQueue();QCOMPARE(transport.requests.size(),1);QVERIFY(!completed.isEmpty());
        client.refreshQueue(true);QTRY_VERIFY(!client.collectionBusy());QCOMPARE(transport.requests.size(),2);
        client.m_track["uri"]="spotify:track:second";
        client.refreshQueue();QTRY_VERIFY(!client.collectionBusy());QCOMPARE(transport.requests.size(),3);
        client.m_track.remove("uri");client.m_track["url"]="https://open.spotify.com/track/third";
        client.refreshQueue();QTRY_VERIFY(!client.collectionBusy());QCOMPARE(transport.requests.size(),4);
        client.m_track["url"]="https://open.spotify.com/track/fourth";
        client.refreshQueue();QTRY_VERIFY(!client.collectionBusy());QCOMPARE(transport.requests.size(),5);
    }
    void favoritesReadSaveAndRemoveUsingTheTrackUri() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        const QString uri="spotify:track:1111111111111111111111";
        client.m_track["uri"]=uri;
        transport.responses.enqueue({200,"[false]"});
        client.refreshFavorite();QTRY_VERIFY(client.favoriteAvailable());
        QVERIFY(!client.favoriteSaved());
        QCOMPARE(transport.requests.first().request.url().path(),QString("/v1/me/library/contains"));
        QCOMPARE(QUrlQuery(transport.requests.first().request.url()).queryItemValue("uris",QUrl::FullyDecoded),uri);
        client.toggleFavorite();
        QCOMPARE(transport.requests.size(),1);
        QVERIFY(client.error().contains("reconnect"));
        client.m_libraryWriteAuthorized=true;
        client.toggleFavorite();QTRY_VERIFY(client.favoriteSaved());
        QCOMPARE(transport.requests[1].method,QByteArray("PUT"));
        QCOMPARE(transport.requests[1].request.url().path(),QString("/v1/me/library"));
        QCOMPARE(QUrlQuery(transport.requests[1].request.url()).queryItemValue("uris",QUrl::FullyDecoded),uri);
        client.toggleFavorite();QTRY_VERIFY(!client.favoriteSaved());
        QCOMPARE(transport.requests[2].method,QByteArray("DELETE"));
    }
    void sdkQueueCanFillAnEmptyStartupQueue() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.setLocalPlaybackDevice("sdk-device");
        const QVariantList next{QVariantMap{{"title","Upcoming"},{"uri","spotify:track:2222222222222222222222"}}};
        client.updateLocalPlaybackState({{"active",true},{"nextTracks",next}});
        QCOMPARE(client.queueItems(),next);
        QVERIFY(transport.requests.isEmpty());
        client.refreshQueue();
        QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(client.queueItems(),next);
        client.updateLocalPlaybackState({{"active",false}});
        QVERIFY(client.queueItems().isEmpty());
    }
    void savedTrackLoadsFromAnIsolatedProfileAndExpires() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto original=qgetenv("PHOSRACK_TEST_MODE");
        struct RestoreTestMode {
            QByteArray value;
            ~RestoreTestMode(){qputenv("PHOSRACK_TEST_MODE",value);}
        } restore{original};
        const QString appId=QString(32,'a');
        const auto configure=[&](SpotifyClient &client){
            client.m_clientId=appId;
            client.m_signedOut=false;
            client.m_settings.beginGroup("resume-fixture");
        };
        // Redirect the actual settings implementation to a temporary file.
        const auto format=QSettings::defaultFormat();
        struct RestoreFormat {
            QSettings::Format format;
            ~RestoreFormat(){QSettings::setDefaultFormat(format);}
        } formatGuard{format};
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());
        qunsetenv("PHOSRACK_TEST_MODE");
        {
            SpotifyClient writer;
            configure(writer);session(writer);
            QVERIFY(writer.m_settings.fileName().startsWith(directory.path()));
            QVERIFY(writer.authorized());
            QVERIFY(!writer.m_signedOut);
            QVERIFY(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"));
            writer.rememberPlayback({{"title","Saved track"},{"uri","spotify:track:1111111111111111111111"},
                {"position",18000},{"duration",90000},{"art","https://example.test/art"}},true);
            QCOMPARE(writer.m_resumeTrack.value("title").toString(),QString("Saved track"));
            writer.m_settings.sync();
            QCOMPARE(writer.m_settings.status(),QSettings::NoError);
            QVERIFY(!writer.resumeTrack().contains("art"));
            QCOMPARE(writer.m_settings.value("spotify/resume/"+appId).toMap().value("title").toString(),QString("Saved track"));
        }
        {
            SpotifyClient restored;
            configure(restored);
            QCOMPARE(restored.m_settings.value("spotify/resume/"+appId).toMap().value("title").toString(),QString("Saved track"));
            QVERIFY(!restored.m_signedOut);
            restored.loadResumeTrack();
            QCOMPARE(restored.resumeTrack().value("title").toString(),QString("Saved track"));
            QCOMPARE(restored.resumeTrack().value("position").toInt(),18000);
            QVERIFY(!restored.authorized());
            auto expired=restored.resumeTrack();
            expired["savedAt"]=QDateTime::currentMSecsSinceEpoch()-25*60*60*1000;
            restored.m_settings.setValue("spotify/resume/"+appId,expired);
            restored.m_resumeTrack.clear();restored.loadResumeTrack();
            QVERIFY(restored.resumeTrack().isEmpty());
            QVERIFY(!restored.m_settings.contains("spotify/resume/"+appId));
        }
    }
    void rememberedTrackStaysPausedUntilPlayIsRequested() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        const QString uri="spotify:track:1111111111111111111111";
        client.rememberPlayback({{"title","Last song"},{"uri",uri},{"position",32000},{"duration",100000}});
        QCOMPARE(client.resumeTrack().value("position").toInt(),32000);
        QVERIFY(!client.track().value("playing").toBool());
        QVERIFY(transport.requests.isEmpty());
        client.setLocalPlaybackDevice("sdk-device");
        for(const auto &request:transport.requests)QCOMPARE(request.method,QByteArray("GET"));
        client.playPause();
        QTRY_VERIFY(!client.busy());
        const auto &play=transport.requests.last();
        QCOMPARE(play.request.url().path(),QString("/v1/me/player/play"));
        const auto body=QJsonDocument::fromJson(play.body).object();
        QCOMPARE(body.value("position_ms").toInt(),32000);
        QCOMPARE(body.value("uris").toArray().first().toString(),uri);
        client.disconnectSpotify();
        QVERIFY(client.resumeTrack().isEmpty());
    }
    void sdkPlaybackDoesNotPollRestForItsOwnState() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.setLocalPlaybackDevice("sdk-device");
        client.updateLocalPlaybackState({{"active",true},{"playing",true},{"uri","spotify:track:test"}});
        QVERIFY(!client.m_poll.isActive());client.pollPlaybackState();
        client.setActive(false);client.setActive(true);QVERIFY(transport.requests.isEmpty());
        client.updateLocalPlaybackState({{"active",true},{"playing",false}});
        QVERIFY(!client.m_poll.isActive());
        client.updateLocalPlaybackState({{"active",false}});
        QVERIFY(client.m_poll.isActive());QCOMPARE(client.m_poll.interval(),30000);
        client.setIntegratedPlayback(false);QCOMPARE(client.m_poll.interval(),15000);
    }
    void queueTrackChangesDuringRequestTriggerOneCoalescedRefresh() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        client.m_track["uri"]="spotify:track:first";
        transport.responses.enqueue({200,R"({"queue":[]})",20});
        client.refreshQueue();
        client.m_track["uri"]="spotify:track:second";
        client.refreshQueue();client.refreshQueue();
        QTRY_COMPARE(transport.requests.size(),2);QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(client.m_queueTrackUri,QString("spotify:track:second"));
        client.refreshQueue();QCOMPARE(transport.requests.size(),2);
    }
    void suspendingRetainsProtectedSessionAndDoesNotRequestAuthorization() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.setLocalPlaybackDevice("sdk-device");
        QSignalSpy changed(&client,&SpotifyClient::authorizationRequiredChanged);
        client.suspendSession();
        QVERIFY(!client.authorized());QVERIFY(!client.localPlaybackReady());
        QVERIFY(!client.authorizationRequired());QVERIFY(changed.isEmpty());
        QVERIFY(transport.requests.isEmpty());
    }
    void refreshesAndRotatesToken() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_refreshToken="test+refresh&=/";client.m_clientId=QString(32,'a');
        transport.responses.enqueue({200,R"({"access_token":"new-access","refresh_token":"new-refresh","expires_in":3600,"scope":"streaming user-read-private user-read-email"})"});
        transport.responses.enqueue({204,{}});transport.responses.enqueue({200,R"({"devices":[]})"});
        client.refreshToken();QTRY_COMPARE(transport.requests.size(),3);
        QCOMPARE(client.m_refreshToken,QByteArray("new-refresh"));
        QCOMPARE(transport.requests[0].request.url().toString(),QString("https://accounts.spotify.com/api/token"));
        QVERIFY(transport.requests[0].body.contains("grant_type=refresh_token"));
        QVERIFY(transport.requests[0].body.contains("refresh_token=test%2Brefresh%26%3D%2F"));
        QVERIFY(!transport.requests[0].body.contains("client_secret"));
        QCOMPARE(transport.requests[1].request.rawHeader("Authorization"),QByteArray("Bearer new-access"));
        QCOMPARE(client.accessTokenForPlayback(),QByteArray("new-access"));
    }
    void legacyRefreshRequiresAnExplicitNewStreamingGrant() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_refreshToken="legacy-refresh";client.m_clientId=QString(32,'a');
        transport.responses.enqueue({200,R"({"access_token":"legacy-access","scope":"user-read-playback-state user-library-read","expires_in":3600})"});
        client.refreshToken();QTRY_VERIFY(!client.m_refreshing);
        QVERIFY(!client.authorized());QVERIFY(client.accessTokenForPlayback().isEmpty());
        QVERIFY(client.m_reauthorizeStreaming);QVERIFY(!client.m_streamingAuthorized);
        QVERIFY(client.error().contains("Connect Spotify again"));
        QCOMPARE(transport.requests.size(),1);
        QVERIFY(!client.m_callback.isListening());
    }
    void knownStreamingGrantSurvivesRefreshWithoutRepeatedScope() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_refreshToken="known-refresh";client.m_streamingAuthorized=true;
        transport.responses.enqueue({200,R"({"access_token":"rotated-access","expires_in":3600})"});
        transport.responses.enqueue({204,{}});transport.responses.enqueue({200,R"({"devices":[]})"});
        QSignalSpy changed(&client,&SpotifyClient::accessTokenChanged);
        client.refreshToken();QTRY_COMPARE(transport.requests.size(),3);
        QCOMPARE(client.accessTokenForPlayback(),QByteArray("rotated-access"));
        QVERIFY(!client.m_reauthorizeStreaming);QCOMPARE(changed.count(),1);
    }
    void unknownRefreshWithoutScopesDoesNotReachTheSdk() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_refreshToken="unknown-refresh";
        transport.responses.enqueue({200,R"({"access_token":"unknown-access","expires_in":3600})"});
        client.refreshToken();QTRY_VERIFY(!client.m_refreshing);
        QVERIFY(client.accessTokenForPlayback().isEmpty());
        QVERIFY(client.m_reauthorizeStreaming);QCOMPARE(transport.requests.size(),1);
    }
    void authorizationGrantKeepsRequestedScopesAfterPlaybackModeChanges() {
        for(const bool requestedStreaming:{false,true}) {
            MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
            // Simulate an authorization begun in one mode, then changed while
            // the browser is open. An omitted scope echoes that original grant.
            client.m_oauthStreamingRequested=requestedStreaming;
            client.m_integratedPlayback=!requestedStreaming;
            transport.responses.enqueue({200,R"({"access_token":"new-grant","expires_in":3600})"});
            transport.responses.enqueue({204,{}});transport.responses.enqueue({200,R"({"devices":[]})"});
            client.requestToken("grant_type=authorization_code");
            QTRY_VERIFY(!client.m_refreshing);
            QCOMPARE(client.m_streamingAuthorized,requestedStreaming);
            QCOMPARE(client.authorized(),requestedStreaming);
            QCOMPARE(client.m_reauthorizeStreaming,!requestedStreaming);
            if(requestedStreaming)QCOMPARE(client.accessTokenForPlayback(),QByteArray("new-grant"));
            else QVERIFY(client.accessTokenForPlayback().isEmpty());
        }
    }
    void integratedPlaybackWaitsForItsOwnDevice() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.m_track["deviceId"]="remote-player";
        client.playPause();client.playItem("spotify:track:1111111111111111111111");
        client.addToQueue("spotify:track:1111111111111111111111");
        QVERIFY(transport.requests.isEmpty());QVERIFY(!client.localPlaybackReady());
        QVERIFY(!client.track().value("canToggle").toBool());
        QVERIFY(client.error().contains("integrated player is not ready"));
    }
    void selectedTrackStartsLocallyWithoutAnExternalPlayer() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.m_track.clear();client.setLocalPlaybackDevice("sdk-device");
        QVERIFY(client.localPlaybackReady());
        QSignalSpy localCommand(&client,&SpotifyClient::localPlaybackCommand);
        client.playItem("spotify:track:1111111111111111111111");QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests.size(),1);
        QCOMPARE(transport.requests[0].request.url().path(),QString("/v1/me/player/play"));
        QCOMPARE(QUrlQuery(transport.requests[0].request.url()).queryItemValue("device_id"),QString("sdk-device"));
        QCOMPARE(QJsonDocument::fromJson(transport.requests[0].body).object().value("uris").toArray().size(),1);
        QCOMPARE(localCommand.count(),1);QCOMPARE(localCommand[0][0].toString(),QString("activate"));
    }
    void playTransfersRemoteContextRatherThanPausingRemotePlayback() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.m_track["deviceId"]="remote-player";client.setLocalPlaybackDevice("sdk-device");
        QVERIFY(!client.track().value("playing").toBool());
        client.playPause();QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests[0].request.url().path(),QString("/v1/me/player"));
        const auto body=QJsonDocument::fromJson(transport.requests[0].body).object();
        QCOMPARE(body.value("device_ids").toArray().first().toString(),QString("sdk-device"));
        QVERIFY(body.value("play").toBool());
    }
    void integratedControlsTargetLocalDeviceAndSdkVolume() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.setLocalPlaybackDevice("sdk-device");
        client.updateLocalPlaybackState({{"active",true},{"playing",true},{"position",10000},{"duration",100000},
            {"canToggle",true},{"canSeek",true},{"canNext",true},{"canPrevious",true},{"canShuffle",true},{"canRepeat",true},{"repeat","off"}});
        client.playPause();QTRY_VERIFY(!client.busy());
        client.seek(20000);QTRY_VERIFY(!client.busy());
        client.next();QTRY_VERIFY(!client.busy());
        client.toggleShuffle();QTRY_VERIFY(!client.busy());
        client.cycleRepeat();QTRY_VERIFY(!client.busy());
        client.addToQueue("spotify:track:1111111111111111111111");QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests.size(),6);
        for(const auto &sent:transport.requests)
            QCOMPARE(QUrlQuery(sent.request.url()).queryItemValue("device_id"),QString("sdk-device"));
        QCOMPARE(transport.requests[0].request.url().path(),QString("/v1/me/player/pause"));
        QSignalSpy localCommand(&client,&SpotifyClient::localPlaybackCommand);
        client.setVolume(150);QCOMPARE(transport.requests.size(),6);
        QCOMPARE(localCommand.count(),1);QCOMPARE(localCommand[0][0].toString(),QString("volume"));
        QCOMPARE(localCommand[0][1].toInt(),100);QCOMPARE(client.track().value("volume").toInt(),100);
        client.clearLocalPlaybackDevice();QVERIFY(!client.localPlaybackReady());
        client.playItem("spotify:track:1111111111111111111111");QCOMPARE(transport.requests.size(),6);
    }
    void remoteTakeoverDoesNotBecomeIntegratedPlayerMetadata() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client,true);
        client.setLocalPlaybackDevice("sdk-device");
        client.updateLocalPlaybackState({{"active",true},{"playing",true},{"title","Local track"},{"canToggle",true}});
        QCOMPARE(client.track().value("title").toString(),QString("Local track"));
        transport.responses.enqueue({200,R"({"is_playing":true,"device":{"id":"remote-device","name":"Remote"},"item":{"name":"Remote track","duration_ms":100000}})"});
        client.refreshState();QTRY_VERIFY(!client.m_polling);
        QVERIFY(client.track().value("title").toString().isEmpty());
        QVERIFY(!client.track().value("playing").toBool());
        client.updateLocalPlaybackState({{"active",true},{"playing",false},{"title","Old local track"}});
        QVERIFY(client.track().value("title").toString().isEmpty());
        client.setIntegratedPlayback(false);
        QCOMPARE(client.track().value("title").toString(),QString("Remote track"));
    }
    void cancelledSessionIgnoresLateResponse() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);
        client.m_refreshToken="test-refresh";
        transport.responses.enqueue({200,R"({"access_token":"late-access","expires_in":3600})",10});
        client.refreshToken();client.disconnectSpotify();QTest::qWait(30);
        QVERIFY(!client.authorized());QVERIFY(client.m_refreshToken.isEmpty());
        QVERIFY(!client.track().value("canToggle").toBool());
        QCOMPARE(transport.requests.size(),1);
    }
    void revocationClearsSession() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        client.m_refreshToken="revoked";
        transport.responses.enqueue({400,R"({"error":"invalid_grant"})"});
        client.refreshToken();QTRY_VERIFY(!client.m_refreshing);
        QVERIFY(!client.authorized());QVERIFY(client.m_refreshToken.isEmpty());
        QVERIFY(client.m_signedOut);
    }
    void searchesEncodedQueriesAndIgnoresOldResults() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({200,R"({"tracks":{"items":[{"uri":"spotify:track:1111111111111111111111","name":"Old","type":"track"}]}})",30});
        transport.responses.enqueue({200,R"({"tracks":{"items":[{"id":"2222222222222222222222","uri":"spotify:track:2222222222222222222222","name":"New","type":"track","artists":[{"name":"Artist"}]}],"next":"next-page"},"albums":{"items":[]},"playlists":{"items":[null]}})"});
        client.search("old");client.search("Björk & friends");
        QTRY_VERIFY(!client.collectionBusy());QTest::qWait(40);
        QCOMPARE(client.searchResults().size(),1);
        QCOMPARE(client.searchResults()[0].toMap().value("title").toString(),QString("New"));
        QCOMPARE(client.searchResults()[0].toMap().value("subtitle").toString(),QString("Artist"));
        QCOMPARE(QUrlQuery(transport.requests[1].request.url()).queryItemValue("q",QUrl::FullyDecoded),QString("Björk & friends"));
        QCOMPARE(QUrlQuery(transport.requests[1].request.url()).queryItemValue("limit"),QString("10"));
        QVERIFY(client.hasMoreSearch());
        transport.responses.enqueue({200,R"({"tracks":{"items":[]}})"});client.search("Björk & friends",true);
        QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(QUrlQuery(transport.requests[2].request.url()).queryItemValue("offset"),QString("10"));
        QVERIFY(!client.hasMoreSearch());
        client.search("");QVERIFY(client.searchResults().isEmpty());
    }
    void libraryPaginationCountsRawItemsAndNormalizesWrappers() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({200,R"({"items":[{"track":{"uri":"spotify:track:1111111111111111111111","name":"Saved","type":"track"}},{"track":null}],"next":"next-page"})"});
        client.loadLibrary("tracks");QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(client.libraryItems().size(),1);QVERIFY(client.hasMoreLibrary());
        transport.responses.enqueue({200,R"({"items":[{"track":{"uri":"spotify:track:2222222222222222222222","name":"More","type":"track"}}],"next":null})"});
        client.loadMoreLibrary();QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(client.libraryItems().size(),2);QVERIFY(!client.hasMoreLibrary());
        QCOMPARE(QUrlQuery(transport.requests[1].request.url()).queryItemValue("offset"),QString("2"));
        transport.responses.enqueue({200,R"({"items":[{"album":{"uri":"spotify:album:3333333333333333333333","name":"Album","type":"album"}}]})"});
        client.loadLibrary("albums");QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(client.libraryItems().size(),1);
        QCOMPARE(client.libraryItems()[0].toMap().value("contextUri").toString(),QString("spotify:album:3333333333333333333333"));
    }
    void playlistUsesCurrentItemsEndpoint() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({200,R"({"items":[{"item":{"uri":"spotify:track:1111111111111111111111","name":"Current item","type":"track"}}]})"});
        client.openCollection("spotify:playlist:4444444444444444444444");QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(transport.requests[0].request.url().path(),QString("/v1/playlists/4444444444444444444444/items"));
        QCOMPARE(client.libraryView(),QString("playlist"));QCOMPARE(client.libraryItems().size(),1);
        QCOMPARE(client.libraryItems()[0].toMap().value("title").toString(),QString("Current item"));
    }
    void collectionsRecoverFromErrorsAndDiscardCancelledReplies() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({403,"{}"});client.loadLibrary("tracks");QTRY_VERIFY(!client.collectionBusy());
        QVERIFY(client.error().contains("permissions"));
        transport.responses.enqueue({200,R"({"items":[{"album":{"uri":"spotify:album:1111111111111111111111","name":"Late","type":"album"}}]})",20});
        client.loadLibrary("albums");client.disconnectSpotify();QTest::qWait(30);
        QVERIFY(client.libraryItems().isEmpty());QVERIFY(!client.collectionBusy());
        client.refreshQueue();QVERIFY(!client.collectionBusy());
    }
    void queueAndPlaybackRequests() {
        MemoryNetwork transport;SpotifyClient client(nullptr,&transport);session(client);
        transport.responses.enqueue({200,R"({"currently_playing":{"name":"Current"},"queue":[{"uri":"spotify:track:1111111111111111111111","name":"Next","type":"track"}]})"});
        client.refreshQueue();QTRY_VERIFY(!client.collectionBusy());
        QCOMPARE(transport.requests[0].request.url().path(),QString("/v1/me/player/queue"));
        QCOMPARE(client.queueItems().size(),1);
        client.playItem("spotify:track:1111111111111111111111");QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests[1].request.url().path(),QString("/v1/me/player/play"));
        QCOMPARE(QJsonDocument::fromJson(transport.requests[1].body).object().value("uris").toArray().first().toString(),QString("spotify:track:1111111111111111111111"));
        client.playItem("spotify:album:2222222222222222222222",true);QTRY_VERIFY(!client.busy());
        QCOMPARE(QJsonDocument::fromJson(transport.requests[2].body).object().value("context_uri").toString(),QString("spotify:album:2222222222222222222222"));
        client.addToQueue("spotify:episode:3333333333333333333333");QTRY_VERIFY(!client.busy());
        QCOMPARE(transport.requests[3].method,QByteArray("POST"));
        QCOMPARE(QUrlQuery(transport.requests[3].request.url()).queryItemValue("uri",QUrl::FullyDecoded),QString("spotify:episode:3333333333333333333333"));
        const auto count=transport.requests.size();
        client.playItem("https://example.org/track");client.addToQueue("spotify:playlist:4444444444444444444444");
        QCOMPARE(transport.requests.size(),count);
    }
};
QTEST_GUILESS_MAIN(SpotifyClientTest)
#include "spotify_test.moc"
