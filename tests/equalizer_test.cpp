// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <cmath>
#include <limits>
#include "equalizer.h"
#include "equalizer_dsp.h"
class EqualizerTest : public QObject {
    Q_OBJECT
private slots:
    void presetsAndCustomState() {
        Equalizer eq;
        QVERIFY(!eq.enabled());QVERIFY(!eq.running());QCOMPARE(eq.gains().size(),10);
        QCOMPARE(eq.preset(),QString("flat"));QCOMPARE(eq.preamp(),0.0);
        for(const auto &item:eq.presets()){
            const auto id=item.toMap().value("id").toString();eq.selectPreset(id);QCOMPARE(eq.preset(),id);
            for(const auto &gain:eq.gains())QVERIFY(std::isfinite(gain.toDouble())&&std::abs(gain.toDouble())<=12);
        }
        eq.selectPreset("bass");QCOMPARE(eq.gains()[0].toDouble(),8.0);QVERIFY(eq.preamp()<0);
        eq.setBand(3,2.5);QCOMPARE(eq.preset(),QString("custom"));QCOMPARE(eq.gains()[3].toDouble(),2.5);
        eq.reset();QCOMPARE(eq.preset(),QString("flat"));QCOMPARE(eq.preamp(),0.0);
        for(const auto &gain:eq.gains())QCOMPARE(gain.toDouble(),0.0);
    }
    void rejectsInvalidControls() {
        Equalizer eq;eq.setBand(-1,3);eq.setBand(10,3);eq.setBand(0,std::numeric_limits<double>::quiet_NaN());
        QCOMPARE(eq.gains()[0].toDouble(),0.0);
        eq.setBand(0,100);QCOMPARE(eq.gains()[0].toDouble(),12.0);
        eq.setBand(1,-100);QCOMPARE(eq.gains()[1].toDouble(),-12.0);
        eq.setPreamp(100);QCOMPARE(eq.preamp(),12.0);
        eq.setPreamp(std::numeric_limits<double>::infinity());QCOMPARE(eq.preamp(),12.0);
        eq.selectPreset("not-a-preset");QCOMPARE(eq.preset(),QString("custom"));
    }
    void workerConfigurationPreservesOutputAndBounds() {
        const auto target=QString("output\" } injected=true {");
        const auto object=QJsonDocument::fromJson(Phosrack::equalizerConfiguration({},0,target).toUtf8()).object();
        QCOMPARE(object.value("target").toString(),target);
        QCOMPARE(object.value("gains").toArray().size(),10);
        QCOMPARE(object.value("preamp").toDouble(),0.);
        const auto controls=QJsonDocument::fromJson(Phosrack::equalizerControls({200.,-200.},100).toUtf8()).object();
        QCOMPARE(controls.value("gains").toArray()[0].toDouble(),12.);
        QCOMPARE(controls.value("gains").toArray()[1].toDouble(),-12.);
        QCOMPARE(controls.value("preamp").toDouble(),12.);
    }
    void actualDspResponseAndStereoIsolation() {
        Phosrack::EqualizerDsp dsp;
        std::array<double,10> gains{};
        const auto measure=[&]{
            double power=0;
            for(int i=0;i<48000;++i){
                const auto sample=.1f*std::sin(2*3.141592653589793*1000*i/48000);
                const auto left=dsp.sample(sample,0),right=dsp.sample(0,1);
                if(i>=24000)power+=left*left;
                if(std::abs(right)>.000001f)return -1.;
            }
            return std::sqrt(power/24000);
        };
        dsp.configure(gains,0);const auto flat=measure();
        gains[5]=6;dsp.configure(gains,0);const auto boosted=measure();
        dsp.configure(gains,-6.020599913);const auto reduced=measure();
        QVERIFY(std::abs(flat-.0707107)<.0001);
        QVERIFY(std::abs(boosted/flat-std::pow(10.,6./20))<.002);
        QVERIFY(std::abs(reduced/boosted-.5)<.002);
        QVERIFY(std::isfinite(dsp.sample(std::numeric_limits<float>::quiet_NaN(),0)));
    }
    void testModeDoesNoIo() {
        Equalizer eq;eq.setEnabled(true);QVERIFY(eq.enabled());QVERIFY(!eq.running());
        eq.setEnabled(false);QVERIFY(!eq.enabled());QVERIFY(!eq.running());
    }
};
QTEST_GUILESS_MAIN(EqualizerTest)
#include "equalizer_test.moc"
