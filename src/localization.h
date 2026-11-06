// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QCoreApplication>
#include <QStringList>
QString selectUiLanguage(const QStringList &languages, const QString &overrideLanguage = {});
void setupLocalization(QCoreApplication &application, const QString &overrideLanguage = {});
