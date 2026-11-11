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
