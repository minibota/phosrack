# Validation record — 0.3.3

Status: release preparation. Reviewed 10 November 2026.

This record separates planned work, automated checks, package/runtime checks and
live account/audio checks. No result from v0.3.2 is counted as a v0.3.3 pass.

## Change scope

- Reduce redundant Web API polling and coalesce queue requests.
- Persist and display Spotify's requested API pause, including quota responses.
- Restore saved sessions on startup; prompt before browser authorization is needed.
- Present initial privacy/terms acceptance and localized OAuth confirmation pages.
- Separate Spotify's ambient visual mode from non-Spotify local audio tools.
- Retain the public Client ID setup and explain developer-account quota scope.

## Results

| Check | Status | Evidence / limits |
| --- | --- | --- |
| Source review | Passed | Independent review of rate-limit, restoration, disconnect and local-audio boundaries; corrections covered by regressions |
| Spotify protocol regression tests | Passed locally | 40 QtTest results with mock transport; local callback binding allowed, no live Spotify quota consumed |
| QML interaction tests | Passed locally | 43 checks, including startup consent, cooldown prompts, settings, fullscreen and ambient animation independent of audio |
| Translation coverage | Passed | Every current tr/qsTr source string exists in Spanish and French catalogs; English source is the fallback |
| OAuth confirmation page | Passed | 9 QtTest results: receipt/denied/invalid states, escaped text, UTF-8 length and security headers; EN/ES/FR rendered in a browser and visually inspected |
| Native Linux build and UI | Passed | Qt 6.11.2 production build, 19/19 CTest checks; actual QML Spotify/local-tool transitions use isolated test backends. Compact layout visually inspected |
| RPM package/runtime | Pending | Fedora 45 x86_64 final artifact |
| DEB package/runtime | Pending | Debian 13 amd64 final artifact |
| Flatpak package/runtime | Pending | KDE / Qt WebEngine BaseApp 6.10 final artifact |
| Windows build/deployment | Pending | Native MSVC build, portable layout and installed application |
| Published checksums | Pending | Local hashes compared with downloaded release assets |
| Live Spotify sign-in/playback | Not run for this release | Respect the existing Spotify cooldown; no retry or Client ID rotation to evade it |
| Audio hardware | Not run for this release | Windows WASAPI/Equalizer APO and platform listening checks are separate |

## Build provenance

Final application commit: pending.

Workflow run URLs, build toolchain versions, Flatpak commit, release asset hashes and
any source differences between package builds must be recorded before publication.

## Important limits of these checks

A mock 429 test verifies client behavior; it cannot establish Spotify's unpublished
quota thresholds or clear a server-side restriction. A browser confirmation page
means the authorization callback was received, not that every API feature is available.

A successful build or media/DRM preflight does not establish authenticated audible
playback. Windows integrated playback remains experimental; the previous Qt 6.8.3
probe supported Opus/Vorbis but not AAC. New runtime results must be recorded above.

Local audio tests must use non-Spotify sound. Spotify mode should keep Phosrack's
monitor and EQ inactive, including startup and mode transitions. This product
boundary does not identify Spotify sound from unrelated applications on a mixed
output. Public-distribution questions remain in [the policy review](spotify-policy.md).
Technical tests are not Spotify approval.
