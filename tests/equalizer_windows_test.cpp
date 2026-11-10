// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <QtTest>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include "equalizer.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
struct TestMode {
    const bool wasSet=qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE");
    const QByteArray previous=qgetenv("PHOSRACK_TEST_MODE");
    explicit TestMode(bool enabled){if(enabled)qputenv("PHOSRACK_TEST_MODE","1");else qunsetenv("PHOSRACK_TEST_MODE");}
    ~TestMode(){if(wasSet)qputenv("PHOSRACK_TEST_MODE",previous);else qunsetenv("PHOSRACK_TEST_MODE");}
};
struct IsolatedSettings {
    QTemporaryDir directory;
    const QSettings::Format previous=QSettings::defaultFormat();
    IsolatedSettings(){QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());}
    ~IsolatedSettings(){QSettings::setDefaultFormat(previous);}
};
QByteArray readFile(const QString &path){QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray();}
bool writeFile(const QString &path,const QByteArray &data){QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(data)==data.size();}
}

class WindowsEqualizerTest : public QObject {
    Q_OBJECT
private slots:
    void spotifyModeBypassesAndRejectsEnable() {
        qputenv("PHOSRACK_TEST_MODE","1");
        Equalizer eq;eq.setEnabled(true);QVERIFY(eq.enabled());
        eq.setPermitted(false);QVERIFY(!eq.enabled());
        eq.setEnabled(true);QVERIFY(!eq.enabled());
        QVERIFY(eq.ensureBypassed());
        eq.setPermitted(true);eq.setEnabled(true);QVERIFY(eq.enabled());
    }

    void previewStateNeverTouchesApoDirectory() {
#ifdef Q_OS_WIN
        TestMode mode(true);
        QTemporaryDir directory;
        Equalizer eq;eq.setApoDirectory(directory.path());eq.selectPreset("bass");
        eq.setEnabled(true);QVERIFY(eq.enabled());QVERIFY(!eq.running());
        QVERIFY(!QFileInfo::exists(QDir(directory.path()).filePath("phosrack.txt")));
        eq.setEnabled(false);QVERIFY(!eq.enabled());
        eq.setApoDirectory(" ");QVERIFY(eq.apoDirectory().isEmpty());
#else
        QSKIP("Equalizer APO integration runs on Windows.");
#endif
    }
    void updatesTemporaryApoConfigurationAndPreservesBytes() {
#ifdef Q_OS_WIN
        IsolatedSettings settings;
        TestMode mode(false);
        QTemporaryDir directory;
        const auto config=QDir(directory.path()).filePath("config.txt");
        const auto managed=QDir(directory.path()).filePath("phosrack.txt");
        QByteArray original="# User caf";original+=char(0xe9);original+="\r\nDevice: Speakers\r\nPreamp: -3 dB\r\n";
        QVERIFY(writeFile(config,original));
        Equalizer eq;eq.setApoDirectory(directory.path());eq.selectPreset("rock");eq.setEnabled(true);
        QVERIFY(eq.enabled());QVERIFY(eq.running());
        QVERIFY(readFile(config).startsWith(original));QCOMPARE(readFile(config+".phosrack-backup"),original);
        QCOMPARE(readFile(managed).count(": ON PK"),10);
        eq.setBand(5,2.5);
        QTRY_VERIFY(readFile(managed).contains("Fc 1000 Hz Gain 2.500 dB"));
        eq.setEnabled(false);QVERIFY(!eq.enabled());QVERIFY(!eq.running());
        QVERIFY(!readFile(managed).contains("Filter"));
        QVERIFY(readFile(config).startsWith(original));
#else
        QSKIP("Equalizer APO integration runs on Windows.");
#endif
    }
    void staleManagedFiltersAreBypassedBeforeSpotify() {
#ifdef Q_OS_WIN
        IsolatedSettings settings;
        TestMode mode(false);
        QTemporaryDir directory;
        const auto managed=QDir(directory.path()).filePath("phosrack.txt");
        QVERIFY(writeFile(managed,"# Phosrack - managed local Equalizer APO filters\nPreamp: 3 dB\nFilter 1: ON PK Fc 1000 Hz Gain 3 dB Q 1.4\n"));
        Equalizer eq;eq.setApoDirectory(directory.path());QVERIFY(!eq.enabled());
        QVERIFY(eq.ensureBypassed());QVERIFY(!readFile(managed).contains("Filter"));
        QVERIFY(readFile(managed).contains("Preamp: 0.000 dB"));
        QVERIFY(writeFile(managed,"# Unrelated manual file\nPreamp: 1 dB\n"));
        QVERIFY(!eq.ensureBypassed());QVERIFY(!eq.error().isEmpty());
        QCOMPARE(readFile(managed),QByteArray("# Unrelated manual file\nPreamp: 1 dB\n"));
#else
        QSKIP("Equalizer APO integration runs on Windows.");
#endif
    }
    void rejectsUtf16WithoutModifyingFiles() {
#ifdef Q_OS_WIN
        IsolatedSettings settings;
        TestMode mode(false);
        QTemporaryDir directory;
        const auto config=QDir(directory.path()).filePath("config.txt");
        const auto original=QByteArray::fromHex("fffe230020005500730065007200");
        QVERIFY(writeFile(config,original));
        Equalizer eq;eq.setApoDirectory(directory.path());eq.setEnabled(true);
        QVERIFY(!eq.enabled());QVERIFY(!eq.error().isEmpty());
        QCOMPARE(readFile(config),original);
        QVERIFY(!QFileInfo::exists(config+".phosrack-backup"));
        QVERIFY(!QFileInfo::exists(QDir(directory.path()).filePath("phosrack.txt")));
#else
        QSKIP("Equalizer APO integration runs on Windows.");
#endif
    }
    void cannotClaimBypassOrChangeFoldersWhenManagedFileIsLocked() {
#ifdef Q_OS_WIN
        IsolatedSettings settings;
        TestMode mode(false);
        QTemporaryDir directory,other;
        QVERIFY(writeFile(QDir(directory.path()).filePath("config.txt"),"# User configuration\n"));
        Equalizer eq;eq.setApoDirectory(directory.path());eq.selectPreset("rock");eq.setEnabled(true);
        QVERIFY(eq.enabled());QVERIFY(eq.running());
        struct FileLock {
            HANDLE handle=INVALID_HANDLE_VALUE;
            ~FileLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}
        } lock;
        const auto filename=QDir(directory.path()).filePath("phosrack.txt").toStdWString();
        lock.handle=CreateFileW(filename.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        QVERIFY(lock.handle!=INVALID_HANDLE_VALUE);
        eq.setEnabled(false);QVERIFY(eq.enabled());QVERIFY(!eq.error().isEmpty());
        eq.setApoDirectory(other.path());QCOMPARE(eq.apoDirectory(),directory.path());
        CloseHandle(lock.handle);lock.handle=INVALID_HANDLE_VALUE;
        eq.setEnabled(false);QVERIFY(!eq.enabled());QVERIFY(eq.error().isEmpty());
#else
        QSKIP("Equalizer APO integration runs on Windows.");
#endif
    }
};
QTEST_GUILESS_MAIN(WindowsEqualizerTest)
#include "equalizer_windows_test.moc"
