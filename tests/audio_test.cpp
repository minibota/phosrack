// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include "audioanalyzer.h"

class AudioTest : public QObject {
    Q_OBJECT
private slots:
    void readsActualDefaultRatherThanFirstSink();
    void neverFallsBackToMicrophoneOrFirstOutput();
    void acceptsJsonStringMetadataAndRejectsInvalidTargets();
    void automaticModeIsSafeInTestsAndCanPause();
    void ignoresMalformedNewerPipeWireParameters();
    void rejectsUnbalancedParametersAndMalformedProperties();
};

void AudioTest::readsActualDefaultRatherThanFirstSink() {
    const auto outputs=Phosrack::parseAudioOutputs(R"JSON([
        {"info":{"props":{"media.class":"Audio/Sink","object.serial":71,"node.name":"first","node.description":"First output"}}},
        {"info":{"props":{"media.class":"Audio/Sink","object.serial":74,"node.name":"actual","node.description":"System output"}}},
        {"type":"PipeWire:Interface:Metadata","props":{"metadata.name":"default"},"metadata":[
            {"subject":0,"key":"default.configured.audio.sink","value":{"name":"first"}},
            {"subject":0,"key":"default.audio.sink","value":{"name":"actual"}}]}
    ])JSON");
    QCOMPARE(outputs.sources.size(),2);
    QCOMPARE(outputs.defaultId,QString("actual"));
    QCOMPARE(outputs.sources[1].toMap().value("serial").toString(),QString("74"));
    QCOMPARE(outputs.sources[1].toMap().value("name").toString(),QString("System output"));
}

void AudioTest::neverFallsBackToMicrophoneOrFirstOutput() {
    const auto outputs=Phosrack::parseAudioOutputs(R"JSON([
        {"info":{"props":{"media.class":"Audio/Source","object.serial":8,"node.name":"input"}}},
        {"info":{"props":{"media.class":"Audio/Sink","object.serial":9,"node.name":"output"}}},
        {"type":"PipeWire:Interface:Metadata","props":{"metadata.name":"default"},"metadata":[
            {"subject":0,"key":"default.audio.sink","value":{"name":"input"}},
            {"subject":0,"key":"default.audio.source","value":{"name":"output"}}]}
    ])JSON");
    QCOMPARE(outputs.sources.size(),1);
    QVERIFY(outputs.defaultId.isEmpty());
    QVERIFY(Phosrack::parseAudioOutputs("not JSON").sources.isEmpty());
}

void AudioTest::acceptsJsonStringMetadataAndRejectsInvalidTargets() {
    const auto outputs=Phosrack::parseAudioOutputs(R"JSON([
        {"info":{"props":{"media.class":"Audio/Sink","object.serial":"bad; command","node.name":"bad"}}},
        {"info":{"props":{"media.class":"Audio/Sink","object.serial":10,"node.name":"output"}}},
        {"type":"PipeWire:Interface:Metadata","props":{"metadata.name":"default"},"metadata":[
            {"subject":0,"key":"default.audio.sink","value":"{\"name\":\"output\"}"}]}
    ])JSON");
    QCOMPARE(outputs.sources.size(),1);
    QCOMPARE(outputs.defaultId,QString("output"));
}

void AudioTest::automaticModeIsSafeInTestsAndCanPause() {
    qputenv("PHOSRACK_TEST_MODE","1");
    AudioAnalyzer analyzer;
    QVERIFY(analyzer.source().isEmpty());
    analyzer.setActive(true);
    QVERIFY(analyzer.active());
    QVERIFY(!analyzer.running());
    analyzer.setSource("alsa_output.test");
    QCOMPARE(analyzer.source(),QString("alsa_output.test"));
    analyzer.setSource("malicious; command");
    QCOMPARE(analyzer.source(),QString("alsa_output.test"));
    analyzer.setSource("");
    QVERIFY(analyzer.source().isEmpty());
    analyzer.stop();
    QVERIFY(!analyzer.active());
    QVERIFY(!analyzer.running());
    QCOMPARE(analyzer.level(),0.);
}

void AudioTest::ignoresMalformedNewerPipeWireParameters() {
    const auto outputs=Phosrack::parseAudioOutputs(R"JSON([
        {"info":{
            "params": {
                "Tag": [{"direction":"Input","text":"escaped quote \" then braces } ] and \\\"params\\\": {"}],
                [ ], [ {"id-000000a1": null} ]
            },
            "props":{"media.class":"Audio/Sink","object.serial":42,"node.name":"output",
                "node.description":"Text with \"params\": { and closing } ]"}
        }},
        {"info":{"props":{"media.class":"Audio/Source","object.serial":99,"node.name":"input"},
            "params":{"unknown":[{"nested":[{"text":"a\\\\\"} ][{"}]}],[],[{"new-pod":null}]}}},
        {"type":"PipeWire:Interface:Metadata","props":{"metadata.name":"default"},"metadata":[
            {"subject":0,"key":"default.audio.sink","value":{"name":"output"}}]}
    ])JSON");
    QCOMPARE(outputs.sources.size(),1);
    QCOMPARE(outputs.defaultId,QString("output"));
    QCOMPARE(outputs.sources.first().toMap().value("serial").toString(),QString("42"));
    QCOMPARE(outputs.sources.first().toMap().value("name").toString(),QString("Text with \"params\": { and closing } ]"));
}

void AudioTest::rejectsUnbalancedParametersAndMalformedProperties() {
    const QList<QByteArray> invalid={
        R"JSON([{"info":{"params":{"nested":[{"text":"unterminated}]}},"props":{}}}])JSON",
        R"JSON([{"info":{"params":{"nested":[} ]},"props":{}}}])JSON",
        R"JSON([{"info":{"params":{"nested":[{}]},"props":{"media.class":"Audio/Sink",}}}])JSON",
        R"JSON([{"info":{"params":{"nested":[{}]},"props":{"media.class":"Audio/Sink"}}])JSON"
    };
    for(const auto &dump:invalid) {
        const auto outputs=Phosrack::parseAudioOutputs(dump);
        QVERIFY(outputs.sources.isEmpty());
        QVERIFY(outputs.defaultId.isEmpty());
    }
}

QTEST_GUILESS_MAIN(AudioTest)
#include "audio_test.moc"
