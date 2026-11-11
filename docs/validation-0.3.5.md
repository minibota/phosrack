# Phosrack 0.3.5 validation

## Linux

All 19 CTest groups passed, including OAuth callback, layout, localization,
marquee overflow and reduced motion, EQ preamp and preset independence,
library favorite operations, playback recovery and SDK queue fallback.

Two excerpts of recorded music were routed to a temporary PipeWire output,
first on the left channel alone and then on the right channel alone. The
active meter responded in both cases and the other channel stayed at zero.
The peak markers cleared during silence. The twelve note bars updated during
both excerpts. The effects were rendered using Phosrack's capture backend and
QML components.

Spotify favorites and playback recovery were tested using simulated Spotify
responses on Linux. No personal library was changed. The playback position and
queue remain subject to the state Spotify exposes. Real Spotify sign-in and
playback, and Windows audio hardware, were not part of this validation.

## Linux packages

The Fedora 45 RPM and Debian 13 DEB passed their build and test jobs in
[Linux CI](https://github.com/minibota/phosrack/actions/runs/43074515443).
The published Flatpak was built from the same source with the KDE 6.10
runtime and upgraded an isolated user installation from 0.3.4. It reported
version 0.3.5 and opened Settings in French and the fullscreen visualizer in
Spanish during smoke tests. Its Flatpak application commit is
`a25ea7325f5be7ff3cc996c7a00029d053badef0273fc174fab123a0d1045d7b`.

## Windows packages

All 18 CTest groups passed in
[Windows CI](https://github.com/minibota/phosrack/actions/runs/43074515540).
CI checked the portable ZIP layout, launched the app from an NSIS installation,
and verified its Start menu shortcut. The Qt WebEngine runtime probe confirmed a
secure context and Widevine MediaKeys creation for Opus and Vorbis. AAC decoding
and AAC MediaKeys were unavailable in the packaged runtime.

The packages use source revision `4c008b7e40bda9d6170af6bec47f0d3926071266`.
See the [package checksums](../releases/v0.3.5/SHA256SUMS.txt).
