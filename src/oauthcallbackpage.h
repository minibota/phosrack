// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QByteArray>

namespace Phosrack {
enum class OAuthPageState { AuthorizationReceived, Denied, InvalidRequest, Expired };
// A complete loopback HTTP response. No OAuth values are accepted or reflected.
QByteArray oauthCallbackResponse(OAuthPageState state);
}
