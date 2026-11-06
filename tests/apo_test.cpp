// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <limits>
#include "apoconfig.h"
class ApoTest : public QObject {
    Q_OBJECT
private slots:
    void realApoFiltersAndBounds() {
        QVariantList gains;for(int i=0;i<10;++i)gains<<0.;
        gains[5]=6.;gains[0]=100.;gains[9]=std::numeric_limits<double>::quiet_NaN();
        const auto text=Phosrack::apoConfiguration(gains,-5,true);
        QCOMPARE(text.count(": ON PK"),10);
        QVERIFY(text.contains("Preamp: -5.000 dB"));
        QVERIFY(text.contains("Fc 1000 Hz Gain 6.000 dB"));
        QVERIFY(text.contains("Fc 31.25 Hz Gain 12.000 dB"));
        QVERIFY(!text.contains("nan",Qt::CaseInsensitive));
        const auto bypass=Phosrack::apoConfiguration(gains,-5,false);
        QVERIFY(!bypass.contains("Filter"));QVERIFY(bypass.contains("Preamp: 0.000 dB"));
    }
    void existingConfigurationPreservedAndIncludeIdempotent() {
        const QString original="# User chain\nDevice: Speakers\nPreamp: -3 dB\n";
        const auto linked=Phosrack::attachApoConfiguration(original);
        QVERIFY(linked.startsWith(original));
        QVERIFY(linked.contains("Device: all\nChannel: all\nInclude: phosrack.txt"));
        QCOMPARE(Phosrack::attachApoConfiguration(linked),linked);
        QCOMPARE(Phosrack::attachApoConfiguration("Include: PHOSRACK.TXT\n"),QString("Include: PHOSRACK.TXT\n"));
        QVERIFY(Phosrack::attachApoConfiguration("# Include: phosrack.txt\n").contains("# BEGIN PHOSRACK"));
    }
    void byteAttachmentPreservesLegacyTextAndRejectsWideEncoding() {
        QByteArray original="# User caf";original+=char(0xe9);original+="\r\nDevice: Speakers\r\nPreamp: -3 dB";
        const auto attached=Phosrack::attachApoConfigurationBytes(original);
        QVERIFY(attached.supported);QVERIFY(attached.contents.startsWith(original));
        QVERIFY(attached.contents.contains("Device: all\nChannel: all\nInclude: phosrack.txt"));
        QCOMPARE(Phosrack::attachApoConfigurationBytes(attached.contents).contents,attached.contents);
        const auto bom=QByteArray::fromHex("efbbbf")+"Include: phosrack.txt\r\n";
        QCOMPARE(Phosrack::attachApoConfigurationBytes(bom).contents,bom);
        QVERIFY(!Phosrack::attachApoConfigurationBytes(QByteArray::fromHex("fffe49006e006300")).supported);
        QVERIFY(!Phosrack::attachApoConfigurationBytes(QByteArray::fromHex("feff0049006e0063")).supported);
        QVERIFY(!Phosrack::attachApoConfigurationBytes(QByteArray("x\0y",3)).supported);
    }
};
QTEST_GUILESS_MAIN(ApoTest)
#include "apo_test.moc"
