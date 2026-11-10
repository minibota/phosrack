// SPDX-License-Identifier: GPL-3.0-or-later
#include "oauthcallbackpage.h"
#include <QCoreApplication>
#include <QString>
#include <QVariant>

namespace Phosrack {
namespace {
QString tr(const char *source) { return QCoreApplication::translate("OAuthCallbackPage", source).toHtmlEscaped(); }
}
QByteArray oauthCallbackResponse(OAuthPageState state) {
    QString title, message, instruction, symbol;
    QByteArray status = "200 OK";
    const bool received = state == OAuthPageState::AuthorizationReceived;
    switch (state) {
    case OAuthPageState::AuthorizationReceived:
        title = tr("Authorization received");
        message = tr("Spotify has sent your authorization to Phosrack.");
        instruction = tr("Return to Phosrack to finish connecting. You can close this tab.");
        symbol = QStringLiteral("&#10003;");
        break;
    case OAuthPageState::Denied:
        title = tr("Sign-in cancelled");
        message = tr("Your Spotify account was not connected.");
        instruction = tr("Return to Phosrack and choose Connect Spotify when you are ready to try again.");
        symbol = QStringLiteral("&#8212;");
        break;
    case OAuthPageState::Expired:
        status = "410 Gone";
        title = tr("This sign-in has expired");
        message = tr("This browser tab belongs to an earlier sign-in attempt.");
        instruction = tr("Return to Phosrack and choose Connect Spotify to open a new sign-in page.");
        symbol = QStringLiteral("&#8635;");
        break;
    case OAuthPageState::InvalidRequest:
        status = "400 Bad Request";
        title = tr("Unable to confirm this sign-in");
        message = tr("Phosrack could not verify this authorization response. Your account has not been connected by this request.");
        instruction = tr("Return to Phosrack and choose Connect Spotify to open a new sign-in page.");
        symbol = QStringLiteral("!");
        break;
    }
    const QString language = QCoreApplication::instance()
        ? QCoreApplication::instance()->property("uiLanguage").toString() : QString();
    const QString safeLanguage = language == "es" || language == "fr" ? language : QStringLiteral("en");
    const QString document = QStringLiteral(R"HTML(<!doctype html>
<html lang="%1">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>%2 · Phosrack</title>
  <style>
    :root { color-scheme: dark; font-family: system-ui, sans-serif; background: #0c1010; color: #e6eddf; }
    * { box-sizing: border-box; }
    body { margin: 0; min-height: 100vh; min-height: 100svh; display: grid; place-items: center; padding: 24px; background: radial-gradient(ellipse at 50%% 15%%, #253626 0, #101716 42%%, #0c1010 80%%); }
    main { width: 100%%; max-width: 560px; border: 1px solid #6b7081; background: #252838; box-shadow: 0 18px 70px #0008, inset 0 0 0 3px #171b28; padding: 5px; }
    header { display: flex; align-items: center; gap: 16px; padding: 15px 20px; background: #1e2231; }
    .brand { color: #baff74; font: 700 19px ui-monospace, monospace; letter-spacing: 3px; text-shadow: 0 0 14px #a5ff6540; }
    .rail { flex: 1; height: 6px; border-block: 1px solid #c6ad65; box-shadow: inset 0 1px #726243; }
    article { margin: 10px; padding: 30px 24px; text-align: center; background: #0a110d; border: 1px solid #59634f; }
    .symbol { display: inline-grid; place-items: center; width: 58px; height: 58px; border: 1px solid %3; color: %3; background: #1b2b18; border-radius: 50%%; font: 32px ui-monospace, monospace; }
    h1 { margin: 23px 0 14px; color: %3; font-size: clamp(23px, 5vw, 29px); line-height: 1.2; font-weight: 650; }
    p { margin: 12px 0; line-height: 1.6; font-size: 16px; }
    .instruction { margin-top: 24px; padding-top: 20px; border-top: 1px solid #33442d; color: #b9c7ae; }
    footer { padding: 5px 18px 13px; text-align: center; color: #a6b397; font-size: 12px; }
    @media (max-width: 420px) { body { padding: 12px; } article { padding: 25px 16px; } }
  </style>
</head>
<body>
  <main>
    <header><span class="brand">PHOSRACK</span><span class="rail" aria-hidden="true"></span></header>
    <article>
      <span class="symbol" aria-hidden="true">%4</span>
      <h1>%2</h1>
      <p>%5</p>
      <p class="instruction">%6</p>
    </article>
    <footer>%7</footer>
  </main>
</body>
</html>
)HTML")
        .arg(safeLanguage, title, received ? QStringLiteral("#baff74") : QStringLiteral("#e9ce8d"),
             symbol, message, instruction, tr("Spotify sign-in · Phosrack"));
    // Percent escapes above keep CSS percentages distinct from QString placeholders.
    QString html = document;
    html.replace(QStringLiteral("%%"), QStringLiteral("%"));
    const QByteArray body = html.toUtf8();
    return "HTTP/1.1 " + status + "\r\nContent-Type: text/html; charset=utf-8\r\n"
        "Cache-Control: no-store\r\nReferrer-Policy: no-referrer\r\nX-Content-Type-Options: nosniff\r\n"
        "Content-Security-Policy: default-src 'none'; style-src 'unsafe-inline'; base-uri 'none'; form-action 'none'; frame-ancestors 'none'\r\n"
        "Connection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
}
}
