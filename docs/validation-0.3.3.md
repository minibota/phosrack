# Validation record — 0.3.3

Release verified 10 November 2026.

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
| RPM package/runtime | Passed | Fedora 45 x86_64 CI: 18/18 CTest checks; extracted final executable reports 0.3.3 and passes French Settings / Spanish fullscreen; header and payload SHA256 digests valid (package unsigned) |
| DEB package/runtime | Passed | Corrected package: 18/18 CI tests; apt resolves qt6-svg-plugins automatically in a clean Debian 13 runtime container. Version, French Settings, Spanish fullscreen and included privacy/terms/notices verified |
| Flatpak package/runtime | Passed | Exact-source local build, installed in an isolated Flatpak root; version, French Settings, Spanish fullscreen, WebEngine helper and included documents verified. Qt Core 6.10.3 / WebEngine 6.10.2, GNU 15.2.0 |
| Windows build/deployment | Passed | Native Qt 6.8.3/MSVC 2022: 18/18 CTest checks; final ZIP layout, silent NSIS install, installed UI launch and Start-menu shortcut verification, and uninstall passed. AAC decoding unavailable; Opus/Vorbis and Widevine MediaKeys available. No live Spotify listening test |
| Release asset checksums | Passed | All five installers plus SHA256SUMS.txt and THIRD_PARTY.md match the sizes and SHA256 digests returned by GitHub for the uploaded assets |
| Live Spotify sign-in/playback | Not run for this release | Respect the existing Spotify cooldown; no retry or Client ID rotation to evade it |
| Audio hardware | Not run for this release | Windows WASAPI/Equalizer APO and platform listening checks are separate |

## Build provenance

Application code: [`c6c702091f8b3ab3c7fdd7a3aadf2d48732be004`](https://github.com/minibota/phosrack/tree/c6c702091f8b3ab3c7fdd7a3aadf2d48732be004).
Debian packaging correction: [`80091d588cfd10cbbda9874bfadb447195adf36a`](https://github.com/minibota/phosrack/tree/80091d588cfd10cbbda9874bfadb447195adf36a), adding the runtime SVG image plugin. Application code is unchanged.

- [Original Linux build](https://github.com/minibota/phosrack/actions/runs/42842329313) produced the published RPM. Its DEB exposed a missing `qt6-svg-plugins` dependency during clean installation; it is not the published DEB.
- [Corrected Debian build](https://github.com/minibota/phosrack/actions/runs/42843464979) uses the corrected dependency list.
- [Windows build](https://github.com/minibota/phosrack/actions/runs/42842329398) produced the EXE and ZIP from the application commit.
- The published Flatpak was built locally from the exact application commit and tested after installation, with app commit `32067e751b6c7b818d7e87d4e61f08b2cf6940248be1fb7ef217477004672072`.
- Normal user installations were not modified. No real Spotify requests were made during these checks.

Package hashes and download links are in the [release index](../releases/v0.3.3/README.md).

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
