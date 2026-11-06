// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QJsonArray>
#include <cmath>
#include <limits>
#include "protocol.h"
#include "spectrum.h"
class CoreTest : public QObject {
    Q_OBJECT
private slots:
    void pkceRfc7636() {
        QCOMPARE(Phosrack::pkceChallenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk"),QByteArray("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
        QCOMPARE(Phosrack::randomToken().size(),64);
        QVERIFY(Phosrack::randomToken()!=Phosrack::randomToken());
    }
    void callbackValidation() {
        const auto valid=Phosrack::parseCallback("GET /callback?code=abc%2B123&state=nonce HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n","nonce");
        QVERIFY(valid.valid);QCOMPARE(valid.code,QString("abc+123"));
        QVERIFY(!Phosrack::parseCallback("GET /callback?code=abc&state=attacker HTTP/1.1\r\n","nonce").valid);
        QVERIFY(!Phosrack::parseCallback("GET /callback?code=abc&state=nonce&state=nonce HTTP/1.1\r\n","nonce").valid);
        QVERIFY(!Phosrack::parseCallback("GET /callback?code=abc&error=denied&state=nonce HTTP/1.1\r\n","nonce").valid);
        QVERIFY(!Phosrack::parseCallback("POST /callback?code=abc&state=nonce HTTP/1.1\r\n","nonce").valid);
        QVERIFY(!Phosrack::parseCallback("GET /other?code=abc&state=nonce HTTP/1.1\r\n","nonce").valid);
        QVERIFY(!Phosrack::parseCallback("GET /callback?code=abc&state= HTTP/1.1\r\n","").valid);
        const auto denied=Phosrack::parseCallback("GET /callback?error=access_denied&state=nonce HTTP/1.1\r\n","nonce");
        QVERIFY(denied.valid);QCOMPARE(denied.error,QString("access_denied"));
    }
    void playbackCapabilities() {
        QJsonObject input{{"is_playing",true},{"progress_ms",9999},{"device",QJsonObject{{"id","test"},{"name","PC"},{"is_restricted",false},{"supports_volume",false}}},
            {"item",QJsonObject{{"name","Track"},{"duration_ms",2000},{"artists",QJsonArray{QJsonObject{{"name","Artist"}}}}}},
            {"actions",QJsonObject{{"disallows",QJsonObject{{"seeking",true},{"skipping_next",true}}}}}};
        const auto state=Phosrack::playbackState(input);
        QCOMPARE(state.value("artist").toString(),QString("Artist"));
        QCOMPARE(state.value("position").toInt(),2000);
        QVERIFY(state.value("canToggle").toBool());
        QVERIFY(!state.value("canSeek").toBool());
        QVERIFY(!state.value("canNext").toBool());
        QVERIFY(!state.value("canVolume").toBool());
        input["device"]=QJsonObject{{"is_restricted",true}};
        QVERIFY(!Phosrack::playbackState(input).value("canControl").toBool());
        const auto empty=Phosrack::playbackState({});
        QVERIFY(!empty.value("canControl").toBool());QVERIFY(empty.value("title").toString().isEmpty());
    }
    void podcastMetadata() {
        const auto state=Phosrack::playbackState({{"item",QJsonObject{{"name","Episode"},{"show",QJsonObject{{"name","Show"}}},{"images",QJsonArray{QJsonObject{{"url","https://example.org/art"}}}}}}});
        QCOMPARE(state.value("artist").toString(),QString("Show"));
        QCOMPARE(state.value("art").toString(),QString("https://example.org/art"));
    }
    void silenceAndInvalidAudio() {
        std::array<float,Phosrack::FftSize> samples{};
        samples[0]=std::numeric_limits<float>::quiet_NaN();
        const auto frame=Phosrack::analyze(samples);
        QCOMPARE(frame.rms,0.f);
        for(auto band:frame.bands)QCOMPARE(band,0.f);
    }
    void detectsOneKilohertz() {
        std::array<float,Phosrack::FftSize> samples{};
        for(int i=0;i<Phosrack::FftSize;++i)samples[i]=.5f*std::sin(2*3.141592653589793*1000*i/48000);
        const auto frame=Phosrack::analyze(samples);
        QVERIFY(std::abs(frame.rms-.3535)<.01);
        int peak=int(std::max_element(frame.bands.begin(),frame.bands.end())-frame.bands.begin());
        const double center=30*std::pow(16000.0/30,(peak+.5)/Phosrack::BandCount);
        QVERIFY2(center>850&&center<1150,qPrintable(QString::number(center)));
        for(auto band:frame.bands)QVERIFY(std::isfinite(band)&&band>=0&&band<=1);
    }
};
QTEST_GUILESS_MAIN(CoreTest)
#include "core_test.moc"
