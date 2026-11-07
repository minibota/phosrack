// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QString>

class PlaybackRuntime : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString widevinePath READ widevinePath WRITE setWidevinePath NOTIFY changed)
    Q_PROPERTY(QString detectedPath READ detectedPath NOTIFY changed)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit PlaybackRuntime(QObject *parent=nullptr);
    QString widevinePath() const { return m_path; }
    QString detectedPath() const;
    bool restartRequired() const { return m_path != m_initialPath; }
    QString error() const { return m_error; }
    void setWidevinePath(const QString &path);
    Q_INVOKABLE void chooseWidevine();
    Q_INVOKABLE void useAutomaticDetection() { setWidevinePath({}); }
    static void configureBeforeApplication();
signals:
    void changed();
private:
    static QString automaticPath();
    QString m_initialPath, m_path, m_error;
};
