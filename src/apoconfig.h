// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
#include <QVariantList>
#include <QByteArray>
namespace Phosrack {
struct ApoAttachment {
    QByteArray contents;
    bool supported = true;
};
QString apoConfiguration(const QVariantList &gains, double preamp, bool enabled);
QString attachApoConfiguration(const QString &existing);
ApoAttachment attachApoConfigurationBytes(const QByteArray &existing);
}
