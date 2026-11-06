// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QJsonObject>
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
    Q_PROPERTY(bool busy READ busy NOTIFY sessionChanged)
    Q_PROPERTY(QString status READ status NOTIFY sessionChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QVariantMap track READ track NOTIFY trackChanged)
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
public:
    explicit SpotifyClient(QObject *parent=nullptr, QNetworkAccessManager *transport=nullptr);
    ~SpotifyClient() override;
    QString clientId() const {return m_clientId;}
    bool hasDefaultClientId() const {return !m_defaultClientId.isEmpty();}
    bool usesDefaultClientId() const {return hasDefaultClientId()&&m_clientId==m_defaultClientId;}
    void setClientId(const QString &id);
    bool authorized() const {return !m_accessToken.isEmpty();}
    bool busy() const {return m_connecting||m_command;}
    QString status() const {return m_status;}
    QString error() const {return m_error;}
    QVariantMap track() const;
    QVariantList devices() const {return m_devices;}
    bool active() const {return m_active;}
    QVariantList searchResults() const {return m_searchResults;}
    QVariantList queueItems() const {return m_queueItems;}
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
    Q_INVOKABLE void connectSpotify();
    Q_INVOKABLE void disconnectSpotify();
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
    Q_INVOKABLE void refreshQueue();
    Q_INVOKABLE void loadLibrary(const QString &kind, bool more=false);
    Q_INVOKABLE void openCollection(const QString &uri);
    Q_INVOKABLE void loadMoreLibrary();
    Q_INVOKABLE void playItem(const QString &uri, bool context=false);
    Q_INVOKABLE void addToQueue(const QString &uri);
signals:
    void clientIdChanged();
    void sessionChanged();
    void trackChanged();
    void devicesChanged();
    void errorChanged();
    void activeChanged();
    void searchChanged();
    void queueChanged();
    void libraryChanged();
    void collectionsChanged();
private:
    friend class SpotifyClientTest;
    using Result=std::function<void(const QJsonObject &,int)>;
    void beginOAuth();
    void acceptCallback();
    void requestToken(const QByteArray &form);
    void refreshToken();
    void api(const QByteArray &method,const QString &path,const QJsonObject &body,Result done,bool command=false,std::function<void()> failed={},std::function<bool()> current={});
    void control(const QByteArray &method,const QString &path,const QJsonObject &body={});
    void fail(const QString &message);
    void persistRefresh();
    void saveClientIdOverride();
    void clearSession(bool forgetStoredToken);
#ifndef Q_OS_WIN
    void withWallet();
#endif
    void forgetRefresh();
    void clearCollections();
    void requestLibrary(bool more);
    QNetworkAccessManager *m_network;
    QTcpServer m_callback;
    QTimer m_poll,m_refresh,m_loginTimeout,m_clock;
    QElapsedTimer m_positionClock;
#ifndef Q_OS_WIN
    QPointer<KWallet::Wallet> m_wallet;
#endif
    QSettings m_settings;
    const QString m_defaultClientId;
    QString m_clientId,m_status,m_error;
    QByteArray m_accessToken,m_refreshToken,m_verifier,m_oauthState;
    QVariantMap m_track;
    QVariantList m_devices;
    QVariantList m_searchResults,m_queueItems,m_libraryItems;
    QString m_searchQuery,m_libraryView,m_libraryPath;
    int m_searchGeneration=0,m_libraryGeneration=0,m_searchOffset=0,m_libraryOffset=0;
    bool m_searchLoading=false,m_queueLoading=false,m_libraryLoading=false;
    bool m_hasMoreSearch=false,m_hasMoreLibrary=false;
    int m_generation=0;
    qint64 m_backoffUntil=0;
    bool m_connecting=false,m_refreshing=false,m_polling=false,m_command=false,m_active=true;
};
