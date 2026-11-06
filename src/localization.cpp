// SPDX-License-Identifier: GPL-3.0-or-later
#include "localization.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QTranslator>
QString selectUiLanguage(const QStringList &languages, const QString &overrideLanguage) {
    const QString preferred = overrideLanguage.isEmpty() ? languages.value(0) : overrideLanguage;
    const QString language = preferred.toLower().replace('_', '-').section('-', 0, 0);
    return language == "es" || language == "fr" ? language : QStringLiteral("en");
}
namespace {
class CatalogTranslator final : public QTranslator {
public:
    CatalogTranslator(const QString &language, QObject *parent) : QTranslator(parent) {
        QFile file(":/i18n/" + language + ".json");
        if(file.open(QIODevice::ReadOnly)) catalog = QJsonDocument::fromJson(file.readAll()).object();
    }
    bool isEmpty() const override { return catalog.isEmpty(); }
    QString translate(const char *, const char *source, const char *, int) const override {
        return catalog.value(QString::fromUtf8(source)).toString();
    }
private:
    QJsonObject catalog;
};
}
void setupLocalization(QCoreApplication &application, const QString &overrideLanguage) {
    const QString language = selectUiLanguage(QLocale::system().uiLanguages(), overrideLanguage);
    application.setProperty("uiLanguage", language);
    if(language == "en") return;
    auto *translator = new CatalogTranslator(language, &application);
    if(!translator->isEmpty()) application.installTranslator(translator);
}
