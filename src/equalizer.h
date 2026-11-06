// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QVariantList>
#include <memory>

namespace Phosrack {
QString equalizerConfiguration(const QVariantList &gains, double preamp, const QString &output);
QString equalizerControls(const QVariantList &gains, double preamp);
}

class Equalizer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QVariantList gains READ gains NOTIFY gainsChanged)
    Q_PROPERTY(double preamp READ preamp WRITE setPreamp NOTIFY gainsChanged)
    Q_PROPERTY(QString preset READ preset NOTIFY gainsChanged)
    Q_PROPERTY(QVariantList presets READ presets CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString apoDirectory READ apoDirectory WRITE setApoDirectory NOTIFY changed)
public:
    explicit Equalizer(QObject *parent=nullptr);
    ~Equalizer() override;
    bool enabled() const { return m_enabled; }
    bool running() const {
#ifdef Q_OS_WIN
        return m_enabled && !m_nodeId.isEmpty();
#else
        return m_filter.state()==QProcess::Running && !m_nodeId.isEmpty();
#endif
    }
    QVariantList gains() const { return m_gains; }
    double preamp() const { return m_preamp; }
    QString preset() const { return m_preset; }
    QVariantList presets() const;
    QString status() const { return m_status; }
    QString error() const { return m_error; }
#ifdef Q_OS_WIN
    QString apoDirectory() const { return m_apoDirectory; }
    void setApoDirectory(const QString &path);
#else
    QString apoDirectory() const { return {}; }
    void setApoDirectory(const QString &) {}
#endif
    void setEnabled(bool enabled);
    void setPreamp(double gain);
    Q_INVOKABLE void setBand(int index, double gain);
    Q_INVOKABLE void selectPreset(const QString &id);
    Q_INVOKABLE void reset();
signals:
    void changed();
    void gainsChanged();
private:
    void discover();
    void acceptDiscovery();
    void launch();
    void stop();
    void fail(const QString &message);
    void sendControls();
    void save();
    QSettings m_settings;
    QProcess m_filter,m_discovery;
    QByteArray m_workerOutput;
    QTimer m_routes,m_update,m_discoveryTimeout,m_startTimeout;
    std::unique_ptr<QTemporaryDir> m_directory;
    QVariantList m_gains;
    double m_preamp=0;
    QString m_preset="flat",m_status,m_error,m_target,m_nodeId;
    QString m_apoDirectory;
    bool m_enabled=false,m_starting=false,m_stopping=false,m_controlsDirty=false;
};
