# Architecture

Phosrack 0.3.4 is a standalone QML + C++ Qt application. The executable registers
SpotifyClient, AudioAnalyzer, Equalizer and Preferences as the Phosrack QML module.
App.qml, rack components, catalogs, icon and the SDK bridge page are embedded Qt
resources. The installed application needs no source checkout or machine-wide QML
plugin path. Python is used only by the earlier isolated feasibility experiment;
it is not a production dependency.

## Rack and settings

App.qml owns the frameless window and its draggable title strip, minimize and close
controls. The rack does not expose maximize or title-strip double-click maximization;
the visualizer has a separate fullscreen window. PhosrackView.qml owns long-lived
session/audio/settings objects and reorders only their presentation panels.
RackPanel.qml implements folding and header/arrow reordering. MusicBrowser.qml
contains search, queue and library views. Lists and Settings scroll independently;
there is no outer scrolling rack.

SettingsDialog.qml provides personal Spotify application setup, Connect Spotify,
playback mode, DRM status/module selection, output override, motion preference and
external Spotify device transfer.
English, Spanish and French use QLocale's first
system UI language, English qsTr()/tr() source strings and embedded JSON catalogs via
QTranslator. Unsupported or missing system language preferences fall back to English.
Stable preference keys and effect identifiers do not depend on translated labels.

## Authorization and Web API

SpotifyClient uses asynchronous HTTPS, PKCE authorization in the user's browser and
the fixed loopback callback `http://127.0.0.1:43821/callback`. Production builds have
an empty `PHOSRACK_SPOTIFY_CLIENT_ID`; each user supplies their own public application
ID, persisted locally. A private build may supply its own default. User overrides
remain separate. No Client Secret or developer-account credentials are embedded.

Refresh tokens use KWallet on Linux or Windows Credential Manager when available;
access tokens remain in memory. Session generations discard replies after cancellation,
while independent search/library generations reject stale collection responses.
Integrated SDK state avoids redundant REST polling; remote state is polled less
frequently, especially when hidden. Queue refreshes happen on navigation, track changes and explicit user actions,
with a shared backend freshness guard instead of periodic polling. A 429 is handled before discarding obsolete collection results,
and its deadline is persisted and never shortened by overlapping responses. The
UI receives an updated remaining wait. Developer-account quota errors are identified
separately from other rate-limit responses. Normalized maps
let QML render tracks, albums and playlists consistently. Spotify's API constrains
collection availability, queue editing and playback capabilities. The saved wait
survives session clearing; no Client ID rotation or automatic bypass is attempted.

Integrated mode requests `streaming`, `user-read-private` and `user-read-email` in
addition to the existing playback/library scopes. A legacy grant is detected once
per configured Client ID and prompts explicit reauthorization. It does not repeatedly
open the browser or silently discard protected stored credentials. Refreshing a token
updates the playback engine through native C++ connections; disconnect/client changes
clear the engine token and device state.

On startup the rack first gates connection on acceptance of the integration terms
and acknowledgement of the privacy notice. `restoreSession()` attempts stored
credentials without opening the browser. Missing/expired credentials or additional
consent produce an in-app authorization notice; user-initiated connection opens the
browser. The callback returns a localized, self-contained HTML page for receipt,
denial or invalid/expired authorization. It does not reflect codes or tokens.
Explicit Disconnect records a persistent sign-out choice before requesting removal
of the saved sign-in. Failed deletion is reported and cannot cause automatic reuse.

## Integrated playback

WebPlaybackController owns an off-the-record QWebEngineProfile and private page for
Spotify's official Web Playback SDK. A loopback server with an ephemeral port and
random path serves the embedded page; it is separate from the OAuth callback above.
No account login takes place in the embedded page. Its QWebChannel bridge supplies
access tokens in memory and returns normalized playback state and SDK events.
The token is not a QML property or invokable API and is not placed in page URLs,
HTML, persistent browser storage or logs. The SDK is loaded from Spotify at runtime.

`Play in Phosrack` is the persisted default mode. A ready SDK device does not take
over another player automatically. Explicit track/context playback targets the local
device ID, and pressing Play can transfer the current session. Transport, shuffle,
repeat and queue requests target the local device; requests wait for readiness instead
of accidentally controlling another player. Local volume uses the SDK. SDK state supplies local progress and metadata;
remote state is refreshed only when needed for an external session or an explicit
transfer. A ready local player does not justify periodic duplicate REST state requests. An empty session can start from Search or Library without an external player.

`Control another Spotify player` disables the integrated engine and keeps the existing
Web API controller for an official app, web player or Spotify Connect device. The
selected external player produces the audio. Closing Phosrack closes the SDK page and
its audio stream; minimizing the rack leaves integrated playback running.

PlaybackRuntime configures an existing compatible Widevine module before WebEngine
initialization. It detects common Chrome locations or uses an explicitly selected
module path. Changing that setting requires restart. Widevine is neither bundled nor
downloaded, and runtime compatibility depends on the platform's WebEngine/codecs/CDM.
See the [versioned validation record](validation-0.3.4.md) for actual platform results.

## Local audio and equalization

The local analyzer and equalizer can run while Spotify is connected. Fullscreen
shows Spotify metadata when available, while the visualizer reads the selected
output's entire local mix. See [policy scope](spotify-policy.md).

The Linux AudioAnalyzer resolves PipeWire's `default.audio.sink` metadata every two
seconds while active. An empty source preference means automatic routing. Explicit
overrides use stable node names; each capture process is pinned to the current
numeric output serial. Only Audio/Sink nodes are offered for capture. Folding the
visualizer or hiding/minimizing its last visible window stops capture. Raw samples
remain in a bounded memory buffer; a Hann-window FFT feeds 48 bands, waveform and
reactive ribbons and the 8 × 6 grid. Notes mode alone runs an additional 8,192-sample
FFT for pitch classes and probable major or minor triads. It smooths pitch estimates
and requires consecutive chord matches before displaying a label. Native route
discovery and capture use fixed commands without a shell.

On Linux, phosrack-eq-worker owns a virtual PipeWire input and physical output stream
and performs the ten-band biquad DSP locally. The parent controls gains over private
stdin; it does not modify a foreign node, restart streams for each change or require
a manager permission. Output is pinned before exposing the virtual sink. Users route
the application they want to equalize to Phosrack EQ.

Windows selects audioanalyzer_windows.cpp for WASAPI loopback and
equalizer_windows.cpp for optional Equalizer APO configuration. Shared monitorformat.h
and apoconfig.cpp helpers have platform-independent fixtures. No Linux PipeWire helper
or KWallet library is required by the Windows executable.

## Packaging

CMake installs the executable, desktop entry, AppStream metadata, icon, license and
documentation, including the privacy notice and integration terms. RPM/DEB use their distribution's Qt WebEngine/WebChannel and other
runtime libraries. ABI-specific native packages are compiled for their target system.
Flatpak compiles the same application with the KDE 6.10 SDK and Qt WebEngine BaseApp.
Windows deploys Qt/compiler DLLs, WebEngineProcess, resource packs, locales and QML
imports before emitting a portable ZIP and NSIS installer. None of these packages
contains Widevine. Build/smoke tests and real authenticated playback are recorded
separately in the validation notes.
