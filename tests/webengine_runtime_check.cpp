#include <QApplication>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineProfile>

#include <functional>
#include <utility>

// Packaging diagnostic only: no Spotify login, tokens, licenses or media requests.
class RuntimePage final : public QWebEnginePage
{
public:
    RuntimePage(QWebEngineProfile *profile, std::function<void(QJsonObject)> report)
        : QWebEnginePage(profile), m_report(std::move(report)) {}

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel, const QString &message,
                                  int, const QString &) override
    {
        const QString prefix = QStringLiteral("PHOSRACK_RUNTIME=");
        if (!message.startsWith(prefix))
            return;
        const auto result = QJsonDocument::fromJson(message.mid(prefix.size()).toUtf8());
        if (result.isObject())
            m_report(result.object());
    }

private:
    std::function<void(QJsonObject)> m_report;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Phosrack WebEngine runtime check"));
    bool reported = false;
    auto finish = [&](QJsonObject result, int exitCode = 0) {
        if (reported)
            return;
        reported = true;
        result.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
        QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Compact) << Qt::endl;
        app.exit(exitCode);
    };

    const QByteArray html = R"HTML(<!doctype html><meta charset="utf-8"><script>
(async () => {
  const types = {aac:'audio/mp4; codecs="mp4a.40.2"',
                 vorbis:'audio/webm; codecs="vorbis"',
                 opus:'audio/webm; codecs="opus"'};
  const audio = document.createElement('audio');
  const result = {secureContext:isSecureContext, codecs:{}, widevine:{}};
  await Promise.all(Object.entries(types).map(async ([name, contentType]) => {
    result.codecs[name] = audio.canPlayType(contentType) || 'unavailable';
    try {
      if (!navigator.requestMediaKeySystemAccess) throw new Error('EMEUnavailable');
      const access = await navigator.requestMediaKeySystemAccess('com.widevine.alpha', [{
        initDataTypes:['cenc'], audioCapabilities:[{contentType, robustness:'SW_SECURE_CRYPTO'}],
        distinctiveIdentifier:'not-allowed', persistentState:'not-allowed', sessionTypes:['temporary']
      }]);
      await access.createMediaKeys();
      result.widevine[name] = 'available';
    } catch (error) { result.widevine[name] = error.name || 'unavailable'; }
  }));
  console.log('PHOSRACK_RUNTIME=' + JSON.stringify(result));
})().catch(error => console.log('PHOSRACK_RUNTIME=' + JSON.stringify({error:error.name})));
</script>)HTML";

    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        QTextStream(stderr) << "Cannot start loopback runtime diagnostic" << Qt::endl;
        return 1;
    }
    QObject::connect(&server, &QTcpServer::newConnection, &app, [&] {
        while (server.hasPendingConnections()) {
            auto *socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket, &html] {
                socket->readAll();
                const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                    "Cache-Control: no-store\r\nConnection: close\r\nContent-Length: "
                    + QByteArray::number(html.size()) + "\r\n\r\n" + html;
                socket->write(response);
                socket->disconnectFromHost();
            });
            QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    QWebEngineProfile profile;
    RuntimePage page(&profile, [&](QJsonObject result) { finish(result); });
    QObject::connect(&page, &QWebEnginePage::loadFinished, &app, [&](bool loaded) {
        if (!loaded)
            finish({{QStringLiteral("error"), QStringLiteral("page-load-failed")}}, 1);
    });
    QTimer::singleShot(15000, &app, [&] {
        finish({{QStringLiteral("error"), QStringLiteral("diagnostic-timeout")}}, 1);
    });
    page.load(QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(server.serverPort())));
    return app.exec();
}
