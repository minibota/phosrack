// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>
#include <QString>

namespace Phosrack::NativeTokenStore {
enum class ReadStatus { Found, Missing, Unavailable };
struct ReadResult {
    ReadStatus status = ReadStatus::Unavailable;
    QByteArray token;
};
// Namespace only this application's credential, keyed by the public Client ID.
QString credentialTarget(const QString &clientId);
ReadResult readRefreshToken(const QString &clientId);
bool writeRefreshToken(const QString &clientId, const QByteArray &token);
bool removeRefreshToken(const QString &clientId);
}
