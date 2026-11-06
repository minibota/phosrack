// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <limits>
#include "monitorformat.h"
#include "spectrum.h"
class MonitorFormatTest : public QObject {
    Q_OBJECT
private slots:
    void decodesMonitorFormatsAndBounds() {
        const float stereo[]={.75f,-.25f};
        QCOMPARE(Phosrack::monitorMonoFrame(reinterpret_cast<const unsigned char*>(stereo),sizeof(stereo),{}),.25f);
        const unsigned char pcm24[]={0,0,0x80,0xff,0xff,0x7f};
        const Phosrack::MonitorFormat packed{Phosrack::MonitorEncoding::Pcm24,2,6,44100};
        QVERIFY(std::abs(Phosrack::monitorMonoFrame(pcm24,sizeof(pcm24),packed))<.000001);
        QCOMPARE(Phosrack::monitorMonoFrame(nullptr,0,packed),0.f);
        QCOMPARE(Phosrack::monitorMonoFrame(pcm24,2,packed),0.f);
        const float invalid[]={std::numeric_limits<float>::quiet_NaN(),.5f};
        QCOMPARE(Phosrack::monitorMonoFrame(reinterpret_cast<const unsigned char*>(invalid),sizeof(invalid),{}),.25f);
        QVERIFY((!Phosrack::MonitorFormat{Phosrack::MonitorEncoding::Pcm32,33,132,48000}.valid()));
    }
    void resamplesKnownSignalToAnalyzerTimebase_data() {
        QTest::addColumn<unsigned>("rate");
        QTest::newRow("44.1k")<<44100u;
        QTest::newRow("48k")<<48000u;
        QTest::newRow("96k")<<96000u;
    }
    void resamplesKnownSignalToAnalyzerTimebase() {
        QFETCH(unsigned,rate);
        Phosrack::MonitorResampler converter(rate);
        std::vector<float> output;
        for(unsigned i=0;i<rate/5;++i)converter.append(.5f*std::sin(2.*3.141592653589793*1000*i/rate),output);
        QVERIFY(output.size()>9500&&output.size()<9700);
        std::array<float,Phosrack::FftSize> samples{};
        std::copy(output.end()-samples.size(),output.end(),samples.begin());
        const auto spectrum=Phosrack::analyze(samples);
        const auto peak=int(std::max_element(spectrum.bands.begin(),spectrum.bands.end())-spectrum.bands.begin());
        const double center=30*std::pow(16000.0/30,(peak+.5)/Phosrack::BandCount);
        QVERIFY(center>850&&center<1150);
        QVERIFY(spectrum.rms>.34&&spectrum.rms<.36);
    }
};
QTEST_GUILESS_MAIN(MonitorFormatTest)
#include "monitorformat_test.moc"
