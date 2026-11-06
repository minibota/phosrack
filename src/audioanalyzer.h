// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "spectrum.h"
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#ifdef Q_OS_WIN
#include <memory>
#endif

namespace Phosrack {
struct AudioOutputs {
    QVariantList sources;
    QString defaultId;
};
AudioOutputs parseAudioOutputs(const QByteArray &dump);
}

class AudioAnalyzer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    // An empty source follows the operating system's default output. Overrides
    // store stable node names; the capture target is always a validated serial.
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourcesChanged)
    Q_PROPERTY(QString resolvedSource READ resolvedSource NOTIFY sourcesChanged)
    Q_PROPERTY(QString outputName READ outputName NOTIFY sourcesChanged)
    Q_PROPERTY(QVariantList bands READ bands NOTIFY frameReady)
    Q_PROPERTY(QVariantList waveform READ waveform NOTIFY frameReady)
    Q_PROPERTY(double level READ level NOTIFY frameReady)
public:
    explicit AudioAnalyzer(QObject *parent=nullptr);
    ~AudioAnalyzer() override;
    bool active() const {return m_active;}
    void setActive(bool value);
#ifdef Q_OS_WIN
    bool running() const;
#else
    bool running() const {return m_capture.state()==QProcess::Running;}
#endif
    QString status() const {return m_status;}
    QVariantList sources() const {return m_sources;}
    QString source() const {return m_source;}
    void setSource(const QString &id);
    QString resolvedSource() const {return m_resolvedSource;}
    QString outputName() const {return m_outputName;}
    QVariantList bands() const {return m_bands;}
    QVariantList waveform() const {return m_wave;}
    double level() const {return m_level;}
    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
signals:
    void changed();
    void sourcesChanged();
    void frameReady();
private:
    void resolveOutput();
    void startCapture();
    void stopCapture();
    void consume();
    void updateFrame();
    void setStatus(const QString &value);
#ifdef Q_OS_WIN
    struct WindowsCapture;
    std::unique_ptr<WindowsCapture> m_windows;
    QTimer m_frames,m_routePoll;
#else
    QProcess m_capture,m_discover;
    QTimer m_frames,m_discoverTimeout,m_routePoll;
#endif
    QString m_status,m_source,m_defaultSource,m_resolvedSource,m_outputName;
    QVariantList m_sources,m_bands,m_wave;
    QByteArray m_pending;
    std::array<float,Phosrack::FftSize> m_ring{};
    int m_write=0,m_fresh=0;
    double m_level=0;
    bool m_requested=false,m_active=false;
};
