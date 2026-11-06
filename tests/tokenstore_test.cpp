// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QUuid>
#include "tokenstore.h"

class TokenStoreTest : public QObject {
    Q_OBJECT
private slots:
    void credentialsAreNamespacedByClientId() {
        const QString client=QString(32,'a');
        QCOMPARE(Phosrack::NativeTokenStore::credentialTarget(client),"Phosrack/Spotify/"+client);
        QVERIFY(Phosrack::NativeTokenStore::credentialTarget("../another-credential").isEmpty());
        QVERIFY(Phosrack::NativeTokenStore::credentialTarget(QString(31,'a')).isEmpty());
    }
    void testModeDoesNotAccessVault() {
        const bool wasSet=qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE");
        const auto previous=qgetenv("PHOSRACK_TEST_MODE");
        qputenv("PHOSRACK_TEST_MODE","1");
        const QString client=QUuid::createUuid().toString(QUuid::Id128);
        const auto result=Phosrack::NativeTokenStore::readRefreshToken(client);
        QVERIFY(result.status==Phosrack::NativeTokenStore::ReadStatus::Missing);
        QVERIFY(result.token.isEmpty());
        QVERIFY(Phosrack::NativeTokenStore::writeRefreshToken(client,"test-only-refresh-token"));
        QVERIFY(Phosrack::NativeTokenStore::removeRefreshToken(client));
        if(wasSet)qputenv("PHOSRACK_TEST_MODE",previous);else qunsetenv("PHOSRACK_TEST_MODE");
    }
    void windowsCredentialRoundTrip() {
#ifdef Q_OS_WIN
        struct VaultMode {
            const bool wasSet=qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE");
            const QByteArray previous=qgetenv("PHOSRACK_TEST_MODE");
            VaultMode(){qunsetenv("PHOSRACK_TEST_MODE");}
            ~VaultMode(){if(wasSet)qputenv("PHOSRACK_TEST_MODE",previous);else qunsetenv("PHOSRACK_TEST_MODE");}
        } mode;
        struct Cleanup {
            const QString client=QUuid::createUuid().toString(QUuid::Id128);
            ~Cleanup(){Phosrack::NativeTokenStore::removeRefreshToken(client);}
        } cleanup;
        const QByteArray token="test-only-refresh-token";
        if(!Phosrack::NativeTokenStore::writeRefreshToken(cleanup.client,token))
            QSKIP("This Windows logon session does not provide a writable credential set.");
        const auto saved=Phosrack::NativeTokenStore::readRefreshToken(cleanup.client);
        QVERIFY(saved.status==Phosrack::NativeTokenStore::ReadStatus::Found);
        QCOMPARE(saved.token,token);
        QVERIFY(Phosrack::NativeTokenStore::removeRefreshToken(cleanup.client));
        QVERIFY(Phosrack::NativeTokenStore::readRefreshToken(cleanup.client).status==Phosrack::NativeTokenStore::ReadStatus::Missing);
#else
        QSKIP("Windows Credential Manager is exercised on Windows.");
#endif
    }
};
QTEST_GUILESS_MAIN(TokenStoreTest)
#include "tokenstore_test.moc"
