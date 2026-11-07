# WebEngine feasibility check — 2026-11-07

## Result so far

**The local Qt WebEngine prerequisite checks pass. Authenticated Spotify playback
is not yet validated.** This is evidence to continue the experiment, not approval
to replace Phosrack's current external-player backend.

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
| Spotify authorization and local SDK device registration | Pending |
| Actual audio, advancing position, pause/resume and seek | Pending |
| Restoring the previous playback device | Pending |

Codec queries and successful media-key creation do not prove that Spotify will
issue a playback license or that audio reaches the speakers. That requires the
interactive Premium-account test described in [README.md](README.md).

## Scope

The experiment drives the native Qt engine through Python bindings. Production
Phosrack remains QML/C++ and has no new WebEngine dependency. The current installed
application, its stored credentials and the published v0.3.1 installers were not
modified by this probe.

This run does not validate QML/C++ integration, Windows playback, a clean machine
without an existing Widevine installation, Flatpak access to DRM components, or
distribution rights for codecs/DRM. Spotify's Premium and application-access
requirements remain in force regardless of the technical result.

Separately, the personal Client ID changes were built in a fresh build directory
with an empty compiled default, and all 17 existing CTest checks passed, including
updated authorization, Settings and localization checks.
