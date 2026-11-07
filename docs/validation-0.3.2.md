# Validation of 0.3.2

This release integrates Spotify's official Web Playback SDK through native C++ Qt
WebEngine, makes personal Client ID setup the default on every platform, and retains
an external-player mode. Additional streaming/profile permissions require existing
users to reconnect. Widevine is detected from an existing installation or selected
by the user; it is not bundled.

## Release verification status

This record separates source checks, package checks and real authenticated playback.
**Release builds and runtime verification are in progress.** Pending rows must be
updated with actual results and source/workflow references before representing them
as completed. Successful historical tests are not results for this release.

| Check | Status | Evidence / scope |
| --- | --- | --- |
| SpotifyClient regression suite | Passed in isolated native test target | 26 QtTest results, including local-device command routing, streaming-grant migration, mode changes during authorization, readiness and remote takeover handling; no live credentials used |
| Full application CTest / QML / localization suite | Passed locally | Native production application build passed all 18 CTest checks; final CI/source references pending |
| Fedora 45 RPM | Pending | Final native build, package metadata and installed/extracted runtime checks required |
| Debian 13 DEB | Pending | Target-distribution build and runtime checks required |
| Windows x64 EXE / ZIP | Pending | Native MSVC build, deployed WebEngine resources and clean deployment smoke checks required |
| Native WebEngine runtime capabilities | Passed | Qt 6.11.2: secure loopback context, AAC/Opus/Vorbis decoding and Widevine MediaKeys creation with the existing Chrome module |
| Flatpak BaseApp prerequisites | Preflight passed | KDE / Qt WebEngine BaseApp 6.10 initialized the native SDK/DRM prerequisites; this is not authenticated audio playback |
| Flatpak final bundle | Pending | Clean build, isolated install, metadata and desktop runtime checks required |
| Native C++ integrated playback on Linux | Passed for basic playback | Production executable in an isolated profile authorized successfully; user confirmed audible audio and pause/resume. Closing the official player, seek and reconnect were not verified in this run |
| Integrated playback inside Flatpak | Pending | Real authorization, local SDK device readiness and audible audio required |
| Windows integrated playback / DRM | Not validated | Requires real Windows runtime with compatible existing Widevine and media codecs |
| Windows WASAPI / Equalizer APO hardware | Not validated | Requires a Windows desktop and separately configured APO installation |
| Final artifact checksums and release publication | Pending | Record final source commit, CI run links and `releases/v0.3.2/` index |

## Fullscreen visualizer performance

The Canvas backing texture is bounded to 1280 × 720 physical pixels, including
HiDPI displays. Audio updates are coalesced to at most approximately 30 frames per
second. Layered translucent strokes preserve the phosphor effect without the
expensive per-bar software blur. Hidden visualizers do not continue rendering.

A synthetic native Wayland/OpenGL benchmark at 2194 × 1146 logical pixels and DPR 2
improved the spectrum from 4 paints in 3.06 seconds to 59 in 3.03 seconds
(approximately 1.3 → 19.5 FPS). Orbit reached 91 paints in 3.03 seconds. These are
rendering measurements on the development machine, not universal frame-rate
promises. Regression checks cover bounded backing size, burst coalescing, hidden
idle behavior and reduced motion.

## Earlier feasibility evidence

The [isolated WebEngine experiment](../experiments/webengine-sdk/RESULTS.md) ran on
Fedora 45 with Qt WebEngine 6.11.2, Chromium 140 and an existing Chrome Widevine module.
It completed authorization, local device registration, playback transfer, pause/resume
and seek; the user confirmed hearing audio. The experiment used Python bindings to
native WebEngine and did not exercise the production C++ integration or final packages.
It supports feasibility, not a claim that every v0.3.2 platform is validated.

## Runtime checks to record

For each package tested with a live account, record the platform and Qt/Chromium/CDM
versions, authorization outcome, local device readiness, a track started directly
from Search or Library, audible output, pause/resume, seek, previous/next and volume.
Confirm that the official Spotify app/web player can be closed, that minimizing the
rack keeps playback alive, and that external mode targets the selected other device.

Also check disconnect/reconnect, the upgrade prompt for older grants, token refresh,
SDK retry, output changes and the compact/fullscreen visualizer. Avoid publishing
Client IDs, OAuth URLs, tokens, account details or raw authenticated logs as evidence.

## Remaining scope

Long-running sessions, network interruptions, hardware hot-unplug, accessibility
readers, Windows audio hardware and DRM compatibility across clean machines need
normal desktop testing beyond headless CI. An installed module path is not evidence
of successful DRM initialization. Build-time codec settings are not an audio test.

Historical [0.3.1 validation](validation-0.3.1.md) and the
[0.3.0 record](validation.md) remain available unchanged. Technical tests do not
establish Spotify policy approval, wider API access or permission to redistribute DRM.
