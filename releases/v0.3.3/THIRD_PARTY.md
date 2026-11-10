# Third-party components in Phosrack 0.3.3

RPM, Windows and Flatpak packages use source [c6c702091f8b3ab3c7fdd7a3aadf2d48732be004](https://github.com/minibota/phosrack/tree/c6c702091f8b3ab3c7fdd7a3aadf2d48732be004).
The DEB uses [80091d588cfd10cbbda9874bfadb447195adf36a](https://github.com/minibota/phosrack/tree/80091d588cfd10cbbda9874bfadb447195adf36a), with the corrected Debian SVG dependency and unchanged application code.
Phosrack is GPL-3.0-or-later. Its source and build/deployment scripts are included
in this repository. Qt modules and their dependencies retain their own licenses.

## Windows

The native Windows workflow uses Qt 6.8.3, MSVC 2022 x64, Qt WebEngine and Chromium:
[build and package run](https://github.com/minibota/phosrack/actions/runs/42842329398).
The portable package permits replacement of its Qt DLLs.

- [Qt 6.8.3 corresponding source](https://download.qt.io/archive/qt/6.8/6.8.3/single/qt-everywhere-src-6.8.3.tar.xz)
- [Qt open-source licensing](https://doc.qt.io/qt-6.8/opensourcelicense.html)
- [Qt WebEngine and Chromium licenses](https://doc.qt.io/qt-6.8/qtwebengine-licensing.html)
- [Qt Windows source-build instructions](https://doc.qt.io/qt-6.8/windows-building.html)

App-local Microsoft Visual C++ runtime DLLs come from the installed MSVC SDK and
retain Microsoft's redistribution license. Equalizer APO is a separate optional
installation and is not bundled. See its [official project](https://sourceforge.net/projects/equalizerapo/).

## Linux

RPM and DEB packages use system-provided Qt, KDE and PipeWire libraries. The Flatpak
uses the KDE 6.10 runtime and Qt WebEngine BaseApp 6.10, including their runtime
components and third-party notices. Source and packaging are available from the
[Qt WebEngine BaseApp project](https://github.com/flathub/io.qt.qtwebengine.BaseApp)
and [KDE runtime project](https://invent.kde.org/packaging/flatpak-kde-runtime).

## Spotify and Widevine

The official Spotify Web Playback SDK is loaded from Spotify at runtime. No Spotify
music, Spotify desktop client or account credentials are included in these packages.
Widevine is neither downloaded nor redistributed by Phosrack; a compatible existing
installation is a separate runtime requirement for integrated playback. Permission
to publish Phosrack's source does not grant Spotify API access or DRM redistribution rights.

## Spotify attribution artwork

The official white Spotify logo is included unmodified for attribution. Spotify
trademarks are not covered by Phosrack’s GPL license. See [source and usage notices](../../THIRD_PARTY.md).
