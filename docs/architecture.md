# Architecture

Phosrack 0.3 is a standalone Qt application. The executable registers the C++
SpotifyClient, AudioAnalyzer, Equalizer and Preferences types as the Phosrack QML module.
App.qml, the rack components, catalogs and icon are embedded Qt resources; the
installed application needs no source checkout or machine-wide QML plugin paths.

App.qml owns the window. PhosrackView.qml owns long-lived session/audio/settings
objects and reorders only the player, equalizer, visualizer and music presentations.
RackPanel.qml implements folding and header/arrow reordering. MusicBrowser.qml
contains search, queue and library views. SettingsDialog.qml contains OAuth
configuration, output override, motion preference and Spotify device transfer.
The window has no outer scrolling view; lists and Settings scroll independently.

SpotifyClient uses asynchronous HTTPS requests, PKCE browser authorization and
a loopback callback. Refresh tokens use KWallet on Linux or Windows Credential Manager when available; access
tokens remain in memory. Session generations discard replies after cancellation,
while independent search/library generations reject stale collection responses.
Playback polling is slower when hidden; rate-limit responses enforce backoff.
Normalized collection maps let QML render tracks, albums and playlists consistently.
Collection and playback capabilities are constrained by Spotify's current API.

The Linux AudioAnalyzer resolves PipeWire's actual default.audio.sink metadata every two
seconds while active. An empty source preference means automatic routing. Explicit
overrides use stable node names; each capture process is pinned to the current
numeric output serial. Only Audio/Sink nodes can be captured. Folding the visualizer
or hiding/minimizing the window stops capture. Raw samples remain in a bounded
in-memory buffer; a Hann-window FFT feeds 48 bands, waveform and reactive ribbons.
Native route discovery and capture execute fixed PipeWire commands without a shell.

Localization uses QLocale's first system UI language, English qsTr()/tr() source
strings and embedded Spanish/French JSON catalogs via QTranslator. Unsupported or
missing system language preferences fall back to English. Stable preference keys
and effect identifiers do not depend on the translated display labels.

CMake installs the executable, desktop entry, AppStream metadata, icon, license and
documentation. CPack builds distribution-native RPM/DEB packages. Flatpak compiles
the same sources inside the KDE SDK. ABI-specific native packages are compiled
separately for their target distributions.


## Local EQ and Windows

On Linux, phosrack-eq-worker owns its PipeWire input/output streams and performs the
ten-band biquad DSP locally. The parent controls gains over private stdin; it does
not modify a foreign PipeWire node, restart the stream for each change or require
a manager permission. The output is pinned before exposing the virtual sink.

Windows selects audioanalyzer_windows.cpp for WASAPI loopback and
equalizer_windows.cpp for optional Equalizer APO configuration. The shared
monitorformat.h and apoconfig.cpp helpers have platform-independent fixtures.
Protected refresh tokens use Windows Credential Manager. No Linux PipeWire helper
or KWallet library is required by the Windows executable.

Native Windows packaging deploys Qt/compiler DLLs and QML imports, validates the
deployed tree without SDK search paths, and emits a portable ZIP and NSIS installer.
