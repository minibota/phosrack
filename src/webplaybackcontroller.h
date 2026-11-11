// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTcpServer>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>

class QTcpSocket;
class QWebEnginePage;
class QWebEngineProfile;
class PlaybackBridge;

// Owns a private, in-memory browser solely for Spotify's official playback SDK.
// OAuth and stored credentials remain owned by SpotifyClient.
class WebPlaybackController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString deviceId READ deviceId NOTIFY readyChanged)
public:
    explicit WebPlaybackController(QObject *parent=nullptr);
    ~WebPlaybackController() override;
    bool enabled() const { return m_enabled; }
    bool ready() const { return m_ready; }
    QString status() const { return m_status; }
    QString error() const { return m_error; }
    QString deviceId() const { return m_deviceId; }
    void setEnabled(bool enabled);
    // Deliberately not a Q_PROPERTY/Q_INVOKABLE: no token is exposed to QML.
    void setAccessToken(const QByteArray &token);
    void reset();
    Q_INVOKABLE void retry();
    Q_INVOKABLE void command(const QString &action, const QVariant &value=QVariant());
signals:
    void enabledChanged();
    void readyChanged();
    void statusChanged();
    void errorChanged();
    void deviceReady(const QString &deviceId);
    void deviceUnavailable();
    void playbackState(const QVariantMap &state);
    void capabilities(const QVariantMap &available);
    void commandFinished(const QString &action, bool ok);
private:
    friend class PlaybackBridge;
    friend class CoreTest;
    static QVariantMap nativeState(const QVariantMap &data,const QString &deviceId);
    void start();
    void stop();
    void acceptConnection();
    void receiveEvent(int generation, const QString &event, const QVariantMap &data);
    void setStatus(const QString &value);
    void setError(const QString &value);
    void unavailable();
    bool m_enabled=true, m_ready=false;
    int m_generation=0;
    QByteArray m_token, m_route, m_origin, m_html, m_channelScript;
    QString m_status, m_error, m_deviceId;
    QUrl m_url;
    QTcpServer m_server;
    QTimer m_startup;
    QSet<QTcpSocket *> m_sockets;
    QWebEngineProfile *m_profile=nullptr;
    QWebEnginePage *m_page=nullptr;
    QPointer<PlaybackBridge> m_bridge;
};
