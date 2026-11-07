# Validation of 0.3.2

This release integrates Spotify's official Web Playback SDK through native C++ Qt
WebEngine, makes personal Client ID setup the default on every platform, and retains
an external-player mode. Additional streaming/profile permissions require existing
users to reconnect. Widevine is detected from an existing installation or selected
by the user; it is not bundled.

## Release verification status

This record separates source checks, package checks and real authenticated playback.
Linux package source: [`a9e9b1359e1789dd2868c9b88c9c5e2e367217a8`](https://github.com/minibota/phosrack/tree/a9e9b1359e1789dd2868c9b88c9c5e2e367217a8).
Windows package source: [`4f048bcf4218d863f002bfd45d6eeb7c30b495f4`](https://github.com/minibota/phosrack/tree/4f048bcf4218d863f002bfd45d6eeb7c30b495f4),
adding the Windows package-root correction and installer checks. The application
source is unchanged; the Linux package prefix remains `/usr`. Later commits add
release assets and document the results.
Successful historical tests are not results for this release.

| Check | Status | Evidence / scope |
| --- | --- | --- |
| SpotifyClient regression suite | Passed in isolated native test target | 26 QtTest results, including local-device command routing, streaming-grant migration, mode changes during authorization, readiness and remote takeover handling; no live credentials used |
| Full application CTest / QML / localization suite | Passed | Native production build: 18/18 CTest checks. RPM and DEB CI: 17/17 each; their containers omit the optional Node bridge check, which passed locally |
| Fedora 45 RPM | Passed | [CI build/test/package](https://github.com/minibota/phosrack/actions/runs/42321879284/job/128248095381); verified metadata, WebEngine dependencies and RPM payload digests; extracted executable reports 0.3.2 and passes French Settings / Spanish fullscreen smoke checks on Fedora |
| Debian 13 DEB | Passed | [CI build/test/package](https://github.com/minibota/phosrack/actions/runs/42321879284/job/128248095454); installed in a clean Debian 13 container with runtime dependencies, without development SDKs; version, French Settings and Spanish fullscreen checks pass |
| Windows x64 EXE / ZIP | Passed | [Corrected native CI](https://github.com/minibota/phosrack/actions/runs/42323150248): 17/17 CTest checks, final ZIP root/resource verification, silent NSIS installation, installed Settings/fullscreen launch with SDK paths removed, actual Start-menu shortcut target verification and uninstall all passed |
| Native WebEngine runtime capabilities | Passed | Qt 6.11.2: secure loopback context, AAC/Opus/Vorbis decoding and Widevine MediaKeys creation with the existing Chrome module |
| Flatpak BaseApp prerequisites | Preflight passed | KDE / Qt WebEngine BaseApp 6.10 initialized the native SDK/DRM prerequisites; this is not authenticated audio playback |
| Flatpak final bundle | Passed | Built locally from the exact release source, installed in an isolated workspace Flatpak root; packaged README matches source, Chromium resources are present, French Settings and Spanish fullscreen checks pass |
| Native C++ integrated playback on Linux | Passed for basic playback | Production executable in an isolated profile authorized successfully; user confirmed audible audio and pause/resume. Closing the official player, seek and reconnect were not verified in this run |
| Integrated playback inside Flatpak | Prerequisites passed; listening not tested | Installed final bundle: Qt 6.10.3, secure loopback context, AAC/Opus/Vorbis decoding and Widevine MediaKeys creation passed. Authenticated Spotify audio has not been tested inside the sandbox |
| Windows integrated playback / DRM | Experimental; listening not tested | Qt 6.8.3 CI probe: secure context, Opus/Vorbis decoding and Widevine MediaKeys creation pass; AAC decoding unavailable and AAC MediaKeys return NotSupportedError. No authenticated Spotify audio test. See [Windows capabilities](windows.md#verified-media-capabilities) |
| Windows WASAPI / Equalizer APO hardware | Not validated | Requires a Windows desktop and separately configured APO installation |
| Final artifact checksums | Passed | All five installers have SHA256 checksums and per-platform source/build references in the [release index](../releases/v0.3.2/README.md) |

The final Flatpak bundle contains application commit
`800b80db2718922c5765b27e2371a909da8c96a9c2ef1238a3ac2a186b972d6b`.
Its WebEngine BaseApp branch is 6.10 and KDE runtime branch is 6.10. The existing
Chrome Widevine module is exposed read-only; no Widevine binary is packaged.
Normal user installations were not modified by package validation.

The player display was also checked with actual Qt geometry at DPR 2 and widths
360, 440 and 700: elapsed/total times remain inside the display with bottom padding,
and long titles/artists elide. The maximize button and title-bar double-click
maximization were removed; window resizing and visualizer fullscreen remain available.

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
