#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Isolated Qt WebEngine/Spotify SDK experiment; no Phosrack settings are used."""
import argparse
import base64
import hashlib
import hmac
import json
import re
import secrets
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlencode, urlsplit

from PySide6.QtCore import QFile, QIODevice, QObject, QTimer, QUrl, Signal, Slot, qVersion
from PySide6.QtGui import QDesktopServices
from PySide6.QtNetwork import QNetworkAccessManager, QNetworkRequest
from PySide6.QtWidgets import QApplication
from PySide6.QtWebChannel import QWebChannel
from PySide6.QtWebEngineCore import QWebEnginePage, QWebEngineProfile
from PySide6.QtWebEngineWidgets import QWebEngineView

ORIGIN = 'http://127.0.0.1:43821'
REDIRECT = ORIGIN + '/callback'
SCOPES = 'streaming user-read-email user-read-private user-read-playback-state user-modify-playback-state'


class Bridge(QObject):
    authorized = Signal()
    codeReceived = Signal(str)
    authFailed = Signal()
    playbackStarted = Signal()
    restoreDone = Signal()

    def __init__(self, args, app):
        super().__init__(app)
        self.args, self.app = args, app
        self.net = QNetworkAccessManager(self)
        self.token = self.refresh = self.state = self.verifier = ''
        self.deadline = 0
        self.lock = threading.Lock()
        self.previous = None
        self.transferred = False
        self.transferring = False
        self.restoring = False
        self.closing = False
        self.events = []
        self.started = time.monotonic()
        self.codeReceived.connect(self.exchange)
        self.authFailed.connect(lambda: self.log_event('authorization_denied'))
        self.refresh_timer = QTimer(self)
        self.refresh_timer.setSingleShot(True)
        self.refresh_timer.timeout.connect(self.refresh_token)

    def log_event(self, name, **data):
        entry = {'elapsed': round(time.monotonic() - self.started, 2), 'event': name, **data}
        self.events.append(entry)
        print(json.dumps(entry, ensure_ascii=True), flush=True)
        if self.args.report:
            Path(self.args.report).write_text(json.dumps(self.events, indent=2) + '\n', encoding='utf-8')

    @Slot(str, str)
    def record(self, name, raw):
        # Never write raw browser errors, URLs, tokens, titles or device identifiers.
        if name not in {'environment', 'codecs', 'widevine_aac', 'widevine_opus',
                        'sdk_loaded', 'sdk_ready', 'sdk_not_ready', 'sdk_connect',
                        'initialization_error', 'authentication_error', 'account_error',
                        'playback_error', 'autoplay_failed', 'state', 'exercise_pause',
                        'exercise_resume', 'exercise_seek', 'exercise_finished', 'js_error'}:
            return
        try:
            data = json.loads(raw)
        except (ValueError, TypeError):
            return
        if not isinstance(data, dict):
            return
        safe = {key: value for key, value in data.items()
                if key in {'available', 'supported', 'keysCreated', 'secureContext', 'eme',
                           'paused', 'position', 'duration', 'hasTrack', 'ok'}
                and isinstance(value, (bool, int, float))}
        if name == 'codecs':
            safe.update({key: data[key] for key in ('aac', 'opus', 'vorbis')
                         if data.get(key) in ('', 'maybe', 'probably')})
        self.log_event(name, **safe)

    @Slot(result=str)
    def getToken(self):
        return self.token

    @Slot(result=bool)
    def exerciseEnabled(self):
        return self.args.exercise

    @Slot()
    def pageReady(self):
        if self.args.login:
            self.login()

    @Slot()
    def login(self):
        if not re.fullmatch(r'[a-fA-F0-9]{32}', self.args.client_id or ''):
            self.log_event('client_id_required')
            return
        with self.lock:
            self.state = secrets.token_urlsafe(32)
            self.verifier = secrets.token_urlsafe(64)
            self.deadline = time.monotonic() + 180
        challenge = base64.urlsafe_b64encode(hashlib.sha256(self.verifier.encode()).digest()).rstrip(b'=').decode()
        query = urlencode({'client_id': self.args.client_id, 'response_type': 'code',
                           'redirect_uri': REDIRECT, 'state': self.state,
                           'code_challenge_method': 'S256', 'code_challenge': challenge,
                           'scope': SCOPES})
        opened = QDesktopServices.openUrl(QUrl('https://accounts.spotify.com/authorize?' + query))
        self.log_event('authorization_browser_opened', ok=opened)

    def request(self, method, url, payload, callback, form=False, bearer=True):
        req = QNetworkRequest(QUrl(url))
        req.setTransferTimeout(15000)
        if bearer:
            req.setRawHeader(b'Authorization', ('Bearer ' + self.token).encode())
        if form:
            req.setHeader(QNetworkRequest.KnownHeaders.ContentTypeHeader, 'application/x-www-form-urlencoded')
            body = urlencode(payload).encode()
        else:
            req.setHeader(QNetworkRequest.KnownHeaders.ContentTypeHeader, 'application/json')
            body = json.dumps(payload).encode() if payload is not None else b''
        reply = self.net.sendCustomRequest(req, method.encode(), body)
        def finished():
            status = reply.attribute(QNetworkRequest.Attribute.HttpStatusCodeAttribute) or 0
            try:
                data = json.loads(bytes(reply.readAll()))
            except (ValueError, TypeError):
                data = {}
            reply.deleteLater()
            callback(int(status), data)
        reply.finished.connect(finished)

    @Slot(str)
    def exchange(self, code):
        verifier, self.verifier = self.verifier, ''
        self.request('POST', 'https://accounts.spotify.com/api/token',
                     {'grant_type': 'authorization_code', 'code': code,
                      'redirect_uri': REDIRECT, 'client_id': self.args.client_id,
                      'code_verifier': verifier}, self.received_token, form=True, bearer=False)

    def refresh_token(self):
        if self.refresh:
            self.request('POST', 'https://accounts.spotify.com/api/token',
                         {'grant_type': 'refresh_token', 'refresh_token': self.refresh,
                          'client_id': self.args.client_id}, self.received_token, form=True, bearer=False)

    def received_token(self, status, data):
        if status != 200 or not data.get('access_token'):
            self.log_event('oauth_token_error', http_status=status)
            return
        first = not self.token
        self.token = data['access_token']
        self.refresh = data.get('refresh_token', self.refresh)
        self.refresh_timer.start(max(30, int(data.get('expires_in', 3600)) - 60) * 1000)
        self.log_event('oauth_authorized' if first else 'oauth_refreshed')
        if first:
            self.authorized.emit()

    @Slot(str)
    def startPlayback(self, device):
        if not self.token or not re.fullmatch(r'[A-Za-z0-9_-]{1,128}', device):
            return
        if self.transferred or self.transferring or self.previous is not None:
            return
        self.transferring = True
        def captured(status, data):
            if self.closing:
                self.transferring = False
                self.app.quit()
                return
            current_device = data.get('device', {}).get('id')
            if status != 200 or not current_device or not data.get('item'):
                self.transferring = False
                self.log_event('existing_playback_required', http_status=status)
                return
            self.previous = {'device': current_device, 'playing': bool(data.get('is_playing'))}
            def transferred(status, _):
                self.transferring = False
                self.log_event('transfer_to_probe', http_status=status)
                if status in (200, 204):
                    self.transferred = True
                    if self.closing:
                        self.restore()
                    else:
                        self.playbackStarted.emit()
                else:
                    self.previous = None
                    if self.closing:
                        self.app.quit()
            self.request('PUT', 'https://api.spotify.com/v1/me/player',
                         {'device_ids': [device], 'play': True}, transferred)
        self.request('GET', 'https://api.spotify.com/v1/me/player', None, captured)

    @Slot()
    def restore(self):
        if self.restoring:
            return
        if not self.transferred or not self.previous:
            self.restoreDone.emit()
            if self.closing:
                self.app.quit()
            return
        self.restoring = True
        original = dict(self.previous)
        def finish(status):
            self.restoring = False
            if status in (200, 204):
                self.transferred = False
                self.previous = None
            self.restoreDone.emit()
            if self.closing:
                self.app.quit()
        def paused(status, _):
            self.log_event('restore_paused_state', http_status=status)
            finish(status)
        def restored(status, _):
            self.log_event('restore_previous_device', http_status=status)
            if status in (200, 204) and not original['playing']:
                # Transfer play=false preserves CURRENT state; explicitly restore pause.
                url = 'https://api.spotify.com/v1/me/player/pause?' + urlencode({'device_id': original['device']})
                QTimer.singleShot(400, lambda: self.request('PUT', url, None, paused))
            else:
                finish(status)
        self.request('PUT', 'https://api.spotify.com/v1/me/player',
                     {'device_ids': [original['device']], 'play': original['playing']}, restored)

    def close(self):
        if self.closing:
            return
        self.closing = True
        self.log_event('probe_closing')
        if not self.transferring:
            self.restore()
        QTimer.singleShot(33000, self.app.quit)


class Page(QWebEnginePage):
    def __init__(self, profile, parent, local_url):
        super().__init__(profile, parent)
        self.local_url = local_url

    def acceptNavigationRequest(self, url, kind, main_frame):
        return not main_frame or url.toString() == self.local_url

    def javaScriptConsoleMessage(self, level, message, line, source):
        pass  # Browser messages can contain auth URLs; structured bridge logs only.


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client-id', default='')
    parser.add_argument('--login', action='store_true')
    parser.add_argument('--exercise', action='store_true', help='Transfer current playback, test transport, then restore device')
    parser.add_argument('--timeout', type=int, default=300)
    parser.add_argument('--report', help='Write sanitized diagnostic events to this JSON file')
    parser.add_argument('--headless', action='store_true', help='Hide the window (use QT_QPA_PLATFORM=offscreen when needed)')
    args = parser.parse_args()
    app = QApplication(sys.argv[:1])
    app.setApplicationName('Phosrack WebEngine Probe')
    app.setQuitOnLastWindowClosed(False)
    bridge = Bridge(args, app)
    bridge.log_event('qt_version', version=qVersion())
    route = '/' + secrets.token_urlsafe(32) + '/'
    html = Path(__file__).with_name('probe.html').read_bytes()
    script = QFile(':/qtwebchannel/qwebchannel.js')
    if not script.open(QIODevice.OpenModeFlag.ReadOnly):
        raise RuntimeError('Qt WebChannel resource unavailable')
    webchannel_js = bytes(script.readAll())
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass
        def send(self, status, data, content_type='text/plain; charset=utf-8'):
            self.send_response(status)
            self.send_header('Content-Type', content_type)
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            self.send_header('Referrer-Policy', 'no-referrer')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        def do_GET(self):
            if self.headers.get('Host') != '127.0.0.1:43821':
                self.send(403, b'Invalid host'); return
            if self.headers.get('Origin') not in (None, ORIGIN):
                self.send(403, b'Invalid origin'); return
            parsed = urlsplit(self.path)
            if parsed.path == route and not parsed.query:
                self.send(200, html, 'text/html; charset=utf-8'); return
            if parsed.path == route + 'qwebchannel.js' and not parsed.query:
                self.send(200, webchannel_js, 'application/javascript'); return
            if parsed.path == '/callback':
                if len(self.path) > 8192:
                    self.send(400, b'Invalid callback'); return
                query = parse_qs(parsed.query)
                state = query.get('state', [''])
                with bridge.lock:
                    valid = (len(state) == 1 and state[0].isascii() and bool(bridge.state) and
                             time.monotonic() < bridge.deadline and
                             hmac.compare_digest(state[0], bridge.state))
                    if valid:
                        bridge.state = ''
                if not valid:
                    self.send(400, b'Invalid or expired authorization state'); return
                code = query.get('code', [])
                if len(code) == 1 and code[0] and 'error' not in query:
                    bridge.codeReceived.emit(code[0])
                    self.send(200, b'Phosrack WebEngine probe authorized. You can close this tab.'); return
                bridge.authFailed.emit()
                self.send(400, b'Authorization was cancelled or denied.'); return
            self.send(404, b'Not found')
    try:
        server = ThreadingHTTPServer(('127.0.0.1', 43821), Handler)
    except OSError:
        bridge.log_event('callback_port_unavailable')
        return 2
    server.daemon_threads = True
    threading.Thread(target=server.serve_forever, daemon=True).start()
    profile = QWebEngineProfile(app)  # No storage name: off-the-record profile.
    local_url = ORIGIN + route
    page = Page(profile, app, local_url)
    channel = QWebChannel(page)
    channel.registerObject('probe', bridge)
    page.setWebChannel(channel)
    class View(QWebEngineView):
        def closeEvent(self, event):
            event.ignore()
            self.hide()
            bridge.close()
    view = View()
    view.setPage(page)
    view.resize(940, 640)
    view.setWindowTitle('Phosrack — isolated WebEngine playback probe')
    if not args.headless:
        view.show()
    page.load(QUrl(local_url))
    QTimer.singleShot(max(1, args.timeout) * 1000, bridge.close)
    result = app.exec()
    server.shutdown()
    bridge.token = bridge.refresh = bridge.verifier = bridge.state = ''
    return result


if __name__ == '__main__':
    raise SystemExit(main())
