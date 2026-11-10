// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QTranslator>
#include "oauthcallbackpage.h"
#include "localization.h"

class CallbackTranslator final : public QTranslator {
public:
    bool isEmpty() const override { return false; }
    QString translate(const char *, const char *source, const char *, int) const override {
        return QByteArray(source) == "Authorization received" ? QString::fromUtf8("Autorización <script>unsafe</script> & recibida") : QString();
    }
};
class OAuthCallbackTest : public QObject {
    Q_OBJECT
private slots:
    void readableStates_data() {
        QTest::addColumn<int>("state");
        QTest::addColumn<QByteArray>("status");
        QTest::addColumn<QByteArray>("title");
        QTest::newRow("received") << int(Phosrack::OAuthPageState::AuthorizationReceived) << QByteArray("200 OK") << QByteArray("Authorization received");
        QTest::newRow("denied") << int(Phosrack::OAuthPageState::Denied) << QByteArray("200 OK") << QByteArray("Sign-in cancelled");
        QTest::newRow("invalid") << int(Phosrack::OAuthPageState::InvalidRequest) << QByteArray("400 Bad Request") << QByteArray("Unable to confirm this sign-in");
        QTest::newRow("expired") << int(Phosrack::OAuthPageState::Expired) << QByteArray("410 Gone") << QByteArray("This sign-in has expired");
    }
    void readableStates() {
        QFETCH(int, state); QFETCH(QByteArray, status); QFETCH(QByteArray, title);
        const auto response = Phosrack::oauthCallbackResponse(Phosrack::OAuthPageState(state));
        QVERIFY(response.startsWith("HTTP/1.1 " + status + "\r\n"));
        const auto split = response.indexOf("\r\n\r\n");
        QVERIFY(split > 0);
        const auto headers = response.left(split);
        const auto body = response.mid(split + 4);
        QVERIFY(headers.contains("Content-Type: text/html; charset=utf-8"));
        QVERIFY(headers.contains("Content-Length: " + QByteArray::number(body.size())));
        QVERIFY(headers.contains("Cache-Control: no-store"));
        QVERIFY(headers.contains("default-src 'none'"));
        QVERIFY(headers.contains("frame-ancestors 'none'"));
        QVERIFY(headers.contains("Referrer-Policy: no-referrer"));
        QVERIFY(body.contains("<h1>" + title + "</h1>"));
        QVERIFY(body.contains("Return to Phosrack"));
        QVERIFY(!body.contains("<script"));
        QVERIFY(!body.contains("https://"));
        QVERIFY(!body.contains("http://"));
    }
    void translationIsEscapedAndLengthCountsUtf8Bytes() {
        CallbackTranslator translator;
        QCoreApplication::installTranslator(&translator);
        QCoreApplication::instance()->setProperty("uiLanguage", "es");
        const auto response = Phosrack::oauthCallbackResponse(Phosrack::OAuthPageState::AuthorizationReceived);
        QCoreApplication::removeTranslator(&translator);
        QCoreApplication::instance()->setProperty("uiLanguage", "en");
        const auto split = response.indexOf("\r\n\r\n");
        const auto body = response.mid(split + 4);
        QVERIFY(body.contains("lang=\"es\""));
        QVERIFY(body.contains(QString::fromUtf8("Autorización &lt;script&gt;unsafe&lt;/script&gt; &amp; recibida").toUtf8()));
        QVERIFY(!body.contains("<script>"));
        QVERIFY(response.left(split).contains("Content-Length: " + QByteArray::number(body.size())));
    }
    void unknownLanguageFallsBackToEnglish() {
        QCoreApplication::instance()->setProperty("uiLanguage", "\" onload=\"unsafe");
        const auto response = Phosrack::oauthCallbackResponse(Phosrack::OAuthPageState::AuthorizationReceived);
        QCoreApplication::instance()->setProperty("uiLanguage", "en");
        QVERIFY(response.contains("<html lang=\"en\">"));
        QVERIFY(!response.contains("onload"));
    }
    void localizedConfirmationPages() {
        auto *application = QCoreApplication::instance();
        for (const auto &language : {QStringLiteral("es"), QStringLiteral("fr")}) {
            setupLocalization(*application, language);
            const auto response = Phosrack::oauthCallbackResponse(Phosrack::OAuthPageState::AuthorizationReceived);
            QVERIFY(response.contains("<html lang=\"" + language.toUtf8() + "\">"));
            const QByteArray title = language == "es" ? QString::fromUtf8("Autorización recibida").toUtf8()
                                                      : QString::fromUtf8("Autorisation reçue").toUtf8();
            QVERIFY(response.contains("<h1>" + title + "</h1>"));
            for (auto *translator : application->findChildren<QTranslator *>()) {
                application->removeTranslator(translator);
                delete translator;
            }
        }
        application->setProperty("uiLanguage", "en");
    }
};
QTEST_GUILESS_MAIN(OAuthCallbackTest)
#include "oauthcallback_test.moc"
