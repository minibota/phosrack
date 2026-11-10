// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <qqml.h>
#include "audioanalyzer.h"
#include "equalizer.h"
#include "localization.h"
#include "preferences.h"
#include "spotifyclient.h"
#include "webplaybackcontroller.h"
#include "playbackruntime.h"

int main(int argc, char **argv) {
    for(int i=1;i<argc;++i) {
        if(QByteArray(argv[i])=="--smoke-test"||QByteArray(argv[i])=="--screenshot")qputenv("PHOSRACK_TEST_MODE","1");
    }
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    PlaybackRuntime::configureBeforeApplication();
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Phosrack");
    QCoreApplication::setOrganizationName("Phosrack");
    QCoreApplication::setApplicationVersion(PHOSRACK_VERSION);
    QGuiApplication::setDesktopFileName("io.phosrack.Phosrack");
    QGuiApplication::setWindowIcon(QIcon(":/io.phosrack.Phosrack.svg"));
    QQuickStyle::setStyle("Basic");
    QCommandLineParser parser;
    parser.setApplicationDescription("Phosrack — modular music rack");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"smoke-test", "Load the application and exit without changing settings."});
    parser.addOption({"screenshot", "Save a screenshot of the application and exit.", "path"});
    parser.addOption({"language", "Override the system language for testing (en/es/fr).", "language"});
    parser.addOption({"settings", "Open Settings on launch (also useful for UI checks)."});
    parser.addOption({"visualizer", "Open the fullscreen visualizer on launch."});
    parser.addOption({"connect", "Connect the configured personal Spotify application on launch."});
    parser.addOption({"playback-diagnostics", "Log playback readiness and transport state without credentials or track identifiers."});
    parser.process(app);
    if (parser.isSet("smoke-test") || parser.isSet("screenshot"))
        qputenv("PHOSRACK_TEST_MODE", "1");
    setupLocalization(app, parser.value("language"));
    qmlRegisterType<SpotifyClient>("Phosrack", 1, 0, "SpotifyClient");
    qmlRegisterType<AudioAnalyzer>("Phosrack", 1, 0, "AudioAnalyzer");
    qmlRegisterType<Equalizer>("Phosrack", 1, 0, "Equalizer");
    qmlRegisterType<Preferences>("Phosrack", 1, 0, "Preferences");
    WebPlaybackController webPlayback;
    PlaybackRuntime playbackRuntime;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("webPlayback",&webPlayback);
    engine.rootContext()->setContextProperty("playbackEnvironment",&playbackRuntime);
    bool qmlWarnings = false;
    QObject::connect(&engine, &QQmlEngine::warnings, &app,
        [&](const QList<QQmlError> &warnings) { if (!warnings.isEmpty()) qmlWarnings = true; });
    engine.load(QUrl(QStringLiteral("qrc:/ui/App.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    auto *session=engine.rootObjects().first()->findChild<SpotifyClient*>("spotifySession");
    if(!session)return 5;
    auto syncPlayback=[&] {
        webPlayback.setEnabled(session->integratedPlayback());
        webPlayback.setAccessToken(session->accessTokenForPlayback());
    };
    QObject::connect(session,&SpotifyClient::accessTokenChanged,&webPlayback,syncPlayback);
    QObject::connect(session,&SpotifyClient::integratedPlaybackChanged,&webPlayback,syncPlayback);
    QObject::connect(&webPlayback,&WebPlaybackController::deviceReady,session,&SpotifyClient::setLocalPlaybackDevice);
    QObject::connect(&webPlayback,&WebPlaybackController::deviceUnavailable,session,&SpotifyClient::clearLocalPlaybackDevice);
    QObject::connect(&webPlayback,&WebPlaybackController::playbackState,session,&SpotifyClient::updateLocalPlaybackState);
    QObject::connect(session,&SpotifyClient::localPlaybackCommand,&webPlayback,&WebPlaybackController::command);
    syncPlayback();
    if(parser.isSet("playback-diagnostics")) {
        QObject::connect(&webPlayback,&WebPlaybackController::deviceReady,&app,[]{qInfo("Phosrack SDK device ready");});
        QObject::connect(&webPlayback,&WebPlaybackController::errorChanged,&app,[&]{if(!webPlayback.error().isEmpty())qInfo().noquote()<<"Phosrack SDK:"<<webPlayback.error();});
        QObject::connect(&webPlayback,&WebPlaybackController::playbackState,&app,[](const QVariantMap &state){
            qInfo().noquote()<<"Phosrack SDK state: playing="<<state.value("playing").toBool()<<"position="<<state.value("position").toInt();
        });
    }
    if(!qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")) {
        QTimer::singleShot(300, &app, [&] {
            if (auto *rack = engine.rootObjects().first()->findChild<QObject*>("rackView"))
                QMetaObject::invokeMethod(rack, parser.isSet("connect") ? "connectSpotify" : "restoreSpotifySession");
        });
    }
    if (parser.isSet("smoke-test")) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (!window || !window->flags().testFlag(Qt::FramelessWindowHint)) return 4;
        auto *rack = window->findChild<QObject*>("rackView");
        auto *eq = window->findChild<Equalizer*>("equalizerBackend");
        auto *monitor = window->findChild<AudioAnalyzer*>("audioMonitor");
        if (!rack || !eq || !monitor || monitor->active() || eq->enabled() || eq->permitted()) return 6;
        // Exercise the actual QML mode boundary with test-mode backends: no
        // system capture, driver changes, wallet access or Spotify requests.
        QMetaObject::invokeMethod(rack, "activateLocalAudioTools");
        if (!eq->permitted()) return 7;
        eq->setEnabled(true);
        QMetaObject::invokeMethod(rack, "restoreSpotifySession");
        if (eq->enabled() || eq->permitted() || monitor->active()) return 8;
    }
    if (parser.isSet("settings")) QTimer::singleShot(150, &app, [&] {
        if (auto *rack = engine.rootObjects().first()->findChild<QObject*>("rackView"))
            QMetaObject::invokeMethod(rack, "openSettings");
    });
    if (parser.isSet("visualizer")) QTimer::singleShot(200, &app, [&] {
        if (auto *rack = engine.rootObjects().first()->findChild<QObject*>("rackView"))
            QMetaObject::invokeMethod(rack, "openVisualizer");
    });
    if (parser.isSet("smoke-test"))
        QTimer::singleShot(700, &app, [&] { app.exit(qmlWarnings ? 3 : 0); });
    if (parser.isSet("screenshot")) {
        QTimer::singleShot(900, &app, [&] {
            auto *window = parser.isSet("visualizer")
                ? engine.rootObjects().first()->findChild<QQuickWindow*>("visualizerFullscreen")
                : qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            const QImage capture = window ? window->grabWindow() : QImage();
            app.exit(!capture.isNull() && capture.save(parser.value("screenshot")) ? 0 : 2);
        });
    }
    return app.exec();
}
