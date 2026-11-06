# Architecture

Phosrack 0.2 is a standalone Qt application. The executable registers the C++
SpotifyClient, AudioAnalyzer and Preferences types as the Phosrack QML module.
App.qml, the rack components, catalogs and icon are embedded Qt resources; the
installed application needs no source checkout or machine-wide QML plugin paths.

App.qml owns the window. PhosrackView.qml owns long-lived session/audio/settings
objects and reorders only the player, visualizer and music presentations.
RackPanel.qml implements folding and header/arrow reordering. MusicBrowser.qml
contains search, queue and library views. SettingsDialog.qml contains OAuth
configuration, output override, motion preference and Spotify device transfer.
The window has no outer scrolling view; lists and Settings scroll independently.

SpotifyClient uses asynchronous HTTPS requests, PKCE browser authorization and
a loopback callback. Refresh tokens are saved in KWallet when available; access
tokens remain in memory. Session generations discard replies after cancellation,
while independent search/library generations reject stale collection responses.
Playback polling is slower when hidden; rate-limit responses enforce backoff.
Normalized collection maps let QML render tracks, albums and playlists consistently.
Collection and playback capabilities are constrained by Spotify's current API.

AudioAnalyzer resolves PipeWire's actual default.audio.sink metadata every two
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
