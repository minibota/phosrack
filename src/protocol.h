// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>
namespace Phosrack {
QByteArray pkceChallenge(const QByteArray &verifier);
QByteArray randomToken();
struct Callback { bool valid = false; QString code; QString error; };
Callback parseCallback(const QByteArray &request, const QString &expectedState);
QVariantMap playbackState(const QJsonObject &object);
}
