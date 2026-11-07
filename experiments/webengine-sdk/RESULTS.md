# WebEngine feasibility check — 2026-11-07

## Result

**Authenticated Spotify SDK playback and the basic transport checks passed on
this Linux host. The user also confirmed that audio was audible.** This supports
continuing with a production integration prototype; it does not yet validate a
replacement for every platform and package supported by Phosrack.

The probe ran on Fedora Linux 45 x86_64 with PySide6 / native Qt WebEngine 6.11.2.
The engine reported Chromium 140. An existing Google Chrome installation provided
Widevine 4.10.3112.0; the experiment did not download or redistribute that module.
It used the normal browser identity and kept DRM, TLS and sandbox checks enabled.

| Check | Observed result |
|---|---|
| Local window and Qt WebChannel bridge | Passed |
| Secure loopback origin | `isSecureContext: true` |
| Encrypted Media Extensions | Available |
| AAC, Vorbis and Opus codec queries | All returned `probably` |
| Widevine access and media-key creation for AAC | Passed |
| Widevine access and media-key creation for Opus | Passed |
| Official Spotify Web Playback SDK script | Loaded successfully |
| External browser authorization launch | Opened successfully |
| Spotify authorization and local SDK device registration | Passed |
| Transfer of playback to the probe | Spotify returned HTTP 204 |
| Advancing playback position | Passed; 4,507 ms of progress over 4.5 seconds |
| Audible output | Confirmed by the user |
| Pause and resume | SDK state changed as requested |
| Seek | Trace shows an approximately five-second jump beyond elapsed playback |
| Restoring the previous playback device | Spotify accepted the request with HTTP 204 |
| Probe shutdown | Exited normally after restoration |

The diagnostic sequence was:

1. OAuth authorization succeeded and the local SDK player became ready.
2. Spotify accepted the transfer to **Phosrack WebEngine Probe**.
3. Unpaused SDK position samples advanced from 34,419 to 35,922, 37,424 and
   38,926 ms at approximately 1.5-second intervals.
4. Pause changed the SDK state to paused; resume changed it back to playing.
5. The trace then advanced from 40,613 ms to 47,872 ms over about 2.3 seconds,
   independently supporting the requested five-second seek.
6. Spotify accepted the transfer back to the previous device, and the probe exited.

There were no recorded SDK initialization, authentication, account, playback or
autoplay errors. The original device's state was not read back after restoration,
so HTTP 204 records acceptance rather than independent verification of that device.
These are basic transport checks, not an exhaustive playback test suite.

After the live run, the probe's seek assertion was tightened to account for elapsed
playback time. Simulated working and ignored seeks, both forward and near the end
of a track, verified that the updated assertion rejects a seek that does nothing.
The live seek result above is supported independently by the captured positions.

## Scope

The experiment drives the native Qt engine through Python bindings. Production
Phosrack remains QML/C++ and has no new WebEngine dependency. The current installed
application, its stored credentials and the published v0.3.1 installers were not
modified by this probe.

This run does not validate QML/C++ integration, Windows playback, a clean machine
without an existing Widevine installation, Flatpak access to DRM components,
long-running token refresh/reconnection, or distribution rights for codecs/DRM.
The test transferred an existing Spotify playback context; the official client
was not deliberately terminated during the run. Spotify's Premium and
application-access requirements remain in force regardless of the technical result.

Separately, the personal Client ID changes were built in a fresh build directory
with an empty compiled default, and all 17 existing CTest checks passed, including
updated authorization, Settings and localization checks.
