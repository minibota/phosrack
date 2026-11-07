// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QQueue>
#include <QUrlQuery>
#include <QTemporaryDir>
#include <cstring>
#include "spotifyclient.h"
#include "spotifyconfig.h"

class MemoryReply : public QNetworkReply {
public:
    MemoryReply(const QNetworkRequest &request,int status,const QByteArray &body,int delay,QObject *parent)
        :QNetworkReply(parent),data(body) {
        setRequest(request);setUrl(request.url());
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);
        setRawHeader("Retry-After","120");
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
    struct Response {int status;QByteArray data;int delay=0;};
    struct Sent {QNetworkRequest request;QByteArray method;QByteArray body;};
    QQueue<Response> responses;
    QList<Sent> requests;
protected:
    QNetworkReply *createRequest(Operation operation,const QNetworkRequest &request,QIODevice *body) override {
        const QByteArray method=operation==PostOperation?QByteArray("POST"):request.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray();
        requests.append({request,method,body?body->readAll():QByteArray()});
        const auto response=responses.isEmpty()?Response{204,{}}:responses.dequeue();
        return new MemoryReply(request,response.status,response.data,response.delay,this);
    }
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
        QSettings settings("Phosrack","Phosrack");
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
