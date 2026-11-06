// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QQueue>
#include <QUrlQuery>
#include <cstring>
#include "spotifyclient.h"

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
    static void session(SpotifyClient &client) {
        client.m_accessToken="test-access";
        client.m_track={{"canControl",true},{"canToggle",true},{"canVolume",true},{"canSeek",true},{"playing",true},{"duration",100000}};
    }
private slots:
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
        transport.responses.enqueue({200,R"({"access_token":"new-access","refresh_token":"new-refresh","expires_in":3600})"});
        transport.responses.enqueue({204,{}});transport.responses.enqueue({200,R"({"devices":[]})"});
        client.refreshToken();QTRY_COMPARE(transport.requests.size(),3);
        QCOMPARE(client.m_refreshToken,QByteArray("new-refresh"));
        QCOMPARE(transport.requests[0].request.url().toString(),QString("https://accounts.spotify.com/api/token"));
        QVERIFY(transport.requests[0].body.contains("grant_type=refresh_token"));
        QVERIFY(transport.requests[0].body.contains("refresh_token=test%2Brefresh%26%3D%2F"));
        QVERIFY(!transport.requests[0].body.contains("client_secret"));
        QCOMPARE(transport.requests[1].request.rawHeader("Authorization"),QByteArray("Bearer new-access"));
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
