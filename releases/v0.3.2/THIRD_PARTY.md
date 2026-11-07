# Third-party components in Phosrack 0.3.2

Linux application packages were built from source
[a9e9b1359e1789dd2868c9b88c9c5e2e367217a8](https://github.com/minibota/phosrack/tree/a9e9b1359e1789dd2868c9b88c9c5e2e367217a8).
Phosrack is GPL-3.0-or-later. Its source and build/deployment scripts are included
in this repository. Qt modules and their dependencies retain their own licenses.

## Windows

Windows packages use source
[`4f048bcf4218d863f002bfd45d6eeb7c30b495f4`](https://github.com/minibota/phosrack/tree/4f048bcf4218d863f002bfd45d6eeb7c30b495f4).
The native Windows workflow uses Qt 6.8.3, MSVC 2022 x64, Qt WebEngine and Chromium:
[build and package run](https://github.com/minibota/phosrack/actions/runs/42323150248).
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
