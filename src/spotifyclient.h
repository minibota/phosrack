// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSettings>
#include <QTcpServer>
#include <QTimer>
#include <QVariantList>
#include <functional>
#ifndef Q_OS_WIN
#include <KWallet>
#endif

class SpotifyClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)
    Q_PROPERTY(bool hasDefaultClientId READ hasDefaultClientId CONSTANT)
    Q_PROPERTY(bool usesDefaultClientId READ usesDefaultClientId NOTIFY clientIdChanged)
    Q_PROPERTY(bool authorized READ authorized NOTIFY sessionChanged)
    Q_PROPERTY(bool authorizationRequired READ authorizationRequired NOTIFY authorizationRequiredChanged)
    Q_PROPERTY(bool rateLimited READ rateLimited NOTIFY rateLimitChanged)
    Q_PROPERTY(qint64 retryAfterSeconds READ retryAfterSeconds NOTIFY rateLimitChanged)
    Q_PROPERTY(QDateTime retryAt READ retryAt NOTIFY rateLimitChanged)
    Q_PROPERTY(QString rateLimitMessage READ rateLimitMessage NOTIFY rateLimitChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY sessionChanged)
    Q_PROPERTY(QString status READ status NOTIFY sessionChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QVariantMap track READ track NOTIFY trackChanged)
    Q_PROPERTY(QVariantMap resumeTrack READ resumeTrack NOTIFY trackChanged)
    Q_PROPERTY(bool favoriteSaved READ favoriteSaved NOTIFY favoriteChanged)
    Q_PROPERTY(bool favoriteAvailable READ favoriteAvailable NOTIFY favoriteChanged)
    Q_PROPERTY(bool favoriteNeedsConsent READ favoriteNeedsConsent NOTIFY favoriteChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchChanged)
    Q_PROPERTY(QVariantList queueItems READ queueItems NOTIFY queueChanged)
    Q_PROPERTY(QVariantList libraryItems READ libraryItems NOTIFY libraryChanged)
    Q_PROPERTY(bool collectionBusy READ collectionBusy NOTIFY collectionsChanged)
    Q_PROPERTY(bool hasMoreSearch READ hasMoreSearch NOTIFY searchChanged)
    Q_PROPERTY(bool hasMoreLibrary READ hasMoreLibrary NOTIFY libraryChanged)
    Q_PROPERTY(QString libraryView READ libraryView NOTIFY libraryChanged)
    Q_PROPERTY(QString credentialStorage READ credentialStorage CONSTANT)
    Q_PROPERTY(bool integratedPlayback READ integratedPlayback WRITE setIntegratedPlayback NOTIFY integratedPlaybackChanged)
    Q_PROPERTY(bool localPlaybackReady READ localPlaybackReady NOTIFY localPlaybackReadyChanged)
public:
    explicit SpotifyClient(QObject *parent=nullptr, QNetworkAccessManager *transport=nullptr);
    ~SpotifyClient() override;
    QString clientId() const {return m_clientId;}
    bool hasDefaultClientId() const {return !m_defaultClientId.isEmpty();}
    bool usesDefaultClientId() const {return hasDefaultClientId()&&m_clientId==m_defaultClientId;}
    void setClientId(const QString &id);
    bool authorized() const {return !m_accessToken.isEmpty();}
    bool authorizationRequired() const {return m_authorizationRequired;}
    bool rateLimited() const {return retryAfterSeconds()>0;}
    qint64 retryAfterSeconds() const;
    QDateTime retryAt() const {return QDateTime::fromMSecsSinceEpoch(m_backoffUntil);}
    QString rateLimitMessage() const;
    bool busy() const {return m_connecting||m_command;}
    QString status() const {return m_status;}
    QString error() const {return rateLimited()?rateLimitMessage():m_error;}
    QVariantMap track() const;
    QVariantMap resumeTrack() const;
    bool favoriteSaved() const {return m_favoriteSaved;}
    bool favoriteAvailable() const {return authorized()&&!favoriteTrackUri().isEmpty()&&!m_favoriteLoading;}
    bool favoriteNeedsConsent() const {return !m_libraryWriteAuthorized;}
    QVariantList devices() const {return m_devices;}
    bool active() const {return m_active;}
    QVariantList searchResults() const {return m_searchResults;}
    QVariantList queueItems() const;
    QVariantList libraryItems() const {return m_libraryItems;}
    bool collectionBusy() const {return m_searchLoading||m_queueLoading||m_libraryLoading;}
    bool hasMoreSearch() const {return m_hasMoreSearch;}
    bool hasMoreLibrary() const {return m_hasMoreLibrary;}
    QString libraryView() const {return m_libraryView;}
    QString credentialStorage() const {
#ifdef Q_OS_WIN
        return QStringLiteral("Windows Credential Manager");
#else
        return QStringLiteral("KWallet");
#endif
    }
    void setActive(bool value);
    bool integratedPlayback() const { return m_integratedPlayback; }
    bool localPlaybackReady() const { return m_integratedPlayback && !m_localDeviceId.isEmpty(); }
    void setIntegratedPlayback(bool value);
    // Native bridge only: never expose access tokens as QML properties or invokables.
    QByteArray accessTokenForPlayback() const { return m_streamingAuthorized ? m_accessToken : QByteArray(); }
    void setLocalPlaybackDevice(const QString &deviceId);
    void clearLocalPlaybackDevice();
    void updateLocalPlaybackState(const QVariantMap &state);
    Q_INVOKABLE void connectSpotify();
    Q_INVOKABLE void restoreSession();
    Q_INVOKABLE void disconnectSpotify();
    Q_INVOKABLE void suspendSession();
    Q_INVOKABLE void resetToDefaultClientId();
    Q_INVOKABLE void refreshState();
    Q_INVOKABLE void refreshDevices();
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(int milliseconds);
    Q_INVOKABLE void setVolume(int value);
    Q_INVOKABLE void toggleShuffle();
    Q_INVOKABLE void cycleRepeat();
    Q_INVOKABLE void transfer(const QString &deviceId);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void search(const QString &query, bool more=false);
    Q_INVOKABLE void refreshQueue(bool force=false);
    Q_INVOKABLE void loadLibrary(const QString &kind, bool more=false);
    Q_INVOKABLE void openCollection(const QString &uri);
    Q_INVOKABLE void loadMoreLibrary();
    Q_INVOKABLE void playItem(const QString &uri, bool context=false);
    Q_INVOKABLE void addToQueue(const QString &uri);
    Q_INVOKABLE void toggleFavorite();
signals:
    void clientIdChanged();
    void sessionChanged();
    void authorizationRequiredChanged();
    void rateLimitChanged();
    void trackChanged();
    void favoriteChanged();
    void devicesChanged();
    void errorChanged();
    void activeChanged();
    void searchChanged();
    void queueChanged();
    void libraryChanged();
    void collectionsChanged();
    void integratedPlaybackChanged();
    void localPlaybackReadyChanged();
    void accessTokenChanged();
    void localPlaybackCommand(const QString &action, const QVariant &value);
private:
    friend class SpotifyClientTest;
    using Result=std::function<void(const QJsonObject &,int)>;
    using DocumentResult=std::function<void(const QJsonDocument &,int)>;
    void startSession(bool interactive);
    void continueAuthorization();
    void setAuthorizationRequired(bool required);
    void setSignedOut(bool signedOut);
    void restoreRateLimit();
    void recordRateLimit(const QString &clientId,const QByteArray &retryAfter,const QJsonObject &response);
    void updateRateLimit();
    void deferTokenRefresh();
    void updatePolling();
    void pollPlaybackState();
    void beginOAuth();
    void acceptCallback();
    void requestToken(const QByteArray &form);
    void refreshToken();
    void api(const QByteArray &method,const QString &path,const QJsonObject &body,Result done,bool command=false,std::function<void()> failed={},std::function<bool()> current={});
    void apiDocument(const QByteArray &method,const QString &path,const QJsonObject &body,DocumentResult done,
                     bool command,std::function<void()> failed={},std::function<bool()> current={});
    QString favoriteTrackUri() const;
    void refreshFavorite();
    void rememberPlayback(const QVariantMap &state, bool force=false);
    void loadResumeTrack();
    void clearResumeTrack();
    void setSdkQueue(const QVariantList &items);
    void control(const QByteArray &method,const QString &path,const QJsonObject &body={});
    void fail(const QString &message);
    bool persistRefresh();
    void saveClientIdOverride();
    void clearSession(bool forgetStoredToken,bool requestAuthorization=true);
#ifndef Q_OS_WIN
    void withWallet();
#endif
    bool forgetRefresh();
    void clearCollections();
    QString queueTrackIdentity() const;
    void requestLibrary(bool more);
    bool playbackCommandAllowed();
    QString playbackPath(const QString &path) const;
    bool hasStreamingGrant(const QString &scopes) const;
    QString streamingGrantKey() const;
    void requireStreamingAuthorization();
    QNetworkAccessManager *m_network;
    QTcpServer m_callback;
    QTimer m_poll,m_refresh,m_loginTimeout,m_clock;
    QElapsedTimer m_positionClock,m_localPositionClock;
#ifndef Q_OS_WIN
    QPointer<KWallet::Wallet> m_wallet;
#endif
    QSettings m_settings;
    const QString m_defaultClientId;
    QString m_clientId,m_status,m_error;
    QString m_localDeviceId;
    QByteArray m_accessToken,m_refreshToken,m_verifier,m_oauthState;
    QVariantMap m_track;
    QVariantMap m_localTrack,m_resumeTrack;
    QVariantList m_sdkQueueItems;
    QString m_favoriteUri;
    int m_favoriteGeneration=0;
    bool m_favoriteKnown=false,m_favoriteSaved=false,m_favoriteLoading=false,m_libraryWriteAuthorized=false;
    qint64 m_resumeSavedAt=0;
    bool m_resumeObserved=false;
    QVariantList m_devices;
    QVariantList m_searchResults,m_queueItems,m_libraryItems;
    QString m_searchQuery,m_libraryView,m_libraryPath;
    int m_searchGeneration=0,m_libraryGeneration=0,m_searchOffset=0,m_libraryOffset=0;
    bool m_searchLoading=false,m_queueLoading=false,m_queueForcePending=false,m_libraryLoading=false;
    bool m_hasMoreSearch=false,m_hasMoreLibrary=false;
    int m_generation=0;
    qint64 m_backoffUntil=0,m_queueFetchedAt=0,m_devicesFetchedAt=0;
    QString m_queueTrackUri;
    bool m_quotaExceeded=false,m_rateLimitPublished=false,m_devicesLoading=false;
    bool m_authorizationRequired=false,m_interactiveLogin=false,m_restoreAttempted=false;
    bool m_signedOut=false;
    bool m_connecting=false,m_refreshing=false,m_polling=false,m_command=false,m_active=true;
    bool m_integratedPlayback=true,m_streamingAuthorized=false,m_reauthorizeStreaming=false,m_oauthStreamingRequested=false;
    bool m_localPlaybackActive=false;
    int m_localVolume=50;
};
