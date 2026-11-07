// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QTranslator>
#include "localization.h"
class LocalizationTest : public QObject {
    Q_OBJECT
private slots:
    void systemLanguageAndFallback() {
        QCOMPARE(selectUiLanguage({"es-ES", "en-US"}), QString("es"));
        QCOMPARE(selectUiLanguage({"fr_CA"}), QString("fr"));
        QCOMPARE(selectUiLanguage({"en-GB"}), QString("en"));
        QCOMPARE(selectUiLanguage({"de-DE", "fr-FR"}), QString("en"));
        QCOMPARE(selectUiLanguage({}), QString("en"));
        QCOMPARE(selectUiLanguage({"es"}, "fr"), QString("fr"));
        QCOMPARE(selectUiLanguage({"fr"}, "unknown"), QString("en"));
    }
    void translatedCatalogs() {
        auto *application = QCoreApplication::instance();
        setupLocalization(*application, "es");
        QCOMPARE(QCoreApplication::translate("PhosrackView", "Settings"), QString("Ajustes"));
        QCOMPARE(QCoreApplication::translate("SettingsDialog", "Spotify application setup"), QString::fromUtf8("Configurar aplicación de Spotify"));
        for(auto *translator:application->findChildren<QTranslator*>()) {
            application->removeTranslator(translator);delete translator;
        }
        setupLocalization(*application, "fr");
        QCOMPARE(QCoreApplication::translate("PhosrackView", "Settings"), QString::fromUtf8("Paramètres"));
        QCOMPARE(QCoreApplication::translate("SettingsDialog", "Spotify application setup"), QString::fromUtf8("Configurer l’application Spotify"));
    }
};
QTEST_GUILESS_MAIN(LocalizationTest)
#include "localization_test.moc"
