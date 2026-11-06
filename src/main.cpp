// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <qqml.h>
#include "audioanalyzer.h"
#include "equalizer.h"
#include "localization.h"
#include "preferences.h"
#include "spotifyclient.h"

int main(int argc, char **argv) {
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
    parser.process(app);
    if (parser.isSet("smoke-test") || parser.isSet("screenshot"))
        qputenv("PHOSRACK_TEST_MODE", "1");
    setupLocalization(app, parser.value("language"));
    qmlRegisterType<SpotifyClient>("Phosrack", 1, 0, "SpotifyClient");
    qmlRegisterType<AudioAnalyzer>("Phosrack", 1, 0, "AudioAnalyzer");
    qmlRegisterType<Equalizer>("Phosrack", 1, 0, "Equalizer");
    qmlRegisterType<Preferences>("Phosrack", 1, 0, "Preferences");
    QQmlApplicationEngine engine;
    bool qmlWarnings = false;
    QObject::connect(&engine, &QQmlEngine::warnings, &app,
        [&](const QList<QQmlError> &warnings) { if (!warnings.isEmpty()) qmlWarnings = true; });
    engine.load(QUrl(QStringLiteral("qrc:/ui/App.qml")));
    if (engine.rootObjects().isEmpty()) return 1;
    if (parser.isSet("smoke-test")) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (!window || !window->flags().testFlag(Qt::FramelessWindowHint)) return 4;
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
