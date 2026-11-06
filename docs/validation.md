# Validation of 0.3.0

This document distinguishes source implementation, compilation, runtime smoke tests
and audio-hardware checks. The release is a development build, not Spotify approval.

## Completed application checks

- Fedora 45 x86_64 Release build, Qt 6.11.2 / KDE Frameworks 6.31.
- All 17 native CTest suites pass: protocol/DSP, Spotify HTTP transport, localization,
  audio routing, QML interactions, EQ, token storage, monitor sample conversion,
  APO configuration and Settings/fullscreen smoke checks in en/es/fr/de.
- Frameless window assertions run in application smoke tests.
- EQ fixtures verify bounds/presets, biquad gain/preamp response and channel isolation.
- Fullscreen QML tests verify open/Escape lifecycle, current/next bindings, queue
  refresh throttling and actual Spotify links.
- Browser tests cover collection navigation, pagination/state restoration and avoiding
  repeated requests on command-session notifications.
- Windows-independent monitor-format tests verify float/PCM decoding and resampling.
  APO tests use temporary files, including preservation/backup/encoding/error cases.
- English, Spanish and French catalogs cover the current source UI strings. The
  unsupported-language check falls back to English.

## Real local DSP proof

Native Linux and the KDE SDK sandbox processed inaudible test streams through the
actual EQ worker. A +6 dB 1 kHz band produced approximately 1.995x amplitude; a
-6.0206 dB preamp produced 0.5x amplitude. Controls act in the owning process over
private stdin, without restarting its streams or granting a PipeWire manager role.
The test used temporary virtual outputs, left the system default unchanged and
saved no raw audio.

The visualization backend also resolved the actual default output and stopped its
capture process cleanly. Idle passive outputs correctly produce no signal; a
diagnostic non-passive capture verified the transport without playing a tone.

## Distribution checks

The distributed RPM, DEB, Flatpak, Windows installer and Windows portable ZIP were
built from source commit `6eddf71adf888b52100137254f78dac6b7474b5e` in GitHub Actions.
The checked-in artifacts are in `releases/v0.3.0/`, with `SHA256SUMS`.

- Fedora 45 RPM: the native CI build and tests passed. The exported application
  passed Settings and fullscreen smoke tests after extraction on Fedora 45. The
  EQ worker resolves system libraries; header/payload SHA256 digests verify.
- Debian 13 DEB: all 17 suites passed with Qt 6.8.2. A minimal Debian image installed
  the package and its declared runtime dependencies without development packages.
  The exact CI DEB then passed Settings/fullscreen smoke tests and created,
  controlled and stopped the real PipeWire EQ output in that minimal runtime.
- KDE 6.10 Flatpak: the exported CI bundle installed in an isolated test profile and
  passed Settings/fullscreen smoke tests. Its installed worker also passed the
  actual +6 dB / 0.5x preamp DSP proof described above. The bundle uses the runtime's
  public PipeWire library/client modules with no host-command execution or extra
  manager permission.
- Windows x64: Qt 6.8.3 / MSVC 2022 on the GitHub-hosted `windows-2022` runner passed
  all 16 suites, including Windows EQ configuration tests. The deployed tree passed
  separate Settings/French and fullscreen/Spanish smoke tests with SDK paths removed.
  CPack produced a real AMD64 executable, portable ZIP and NSIS installer. ZIP
  integrity was checked; the installer was built, not interactively installed.

Build records: [Linux run](https://github.com/minibota/phosrack/actions/runs/42306148198)
and [Windows run](https://github.com/minibota/phosrack/actions/runs/42306148235).

## Remaining manual checks

No Spotify Client ID has been configured for the user's real account yet. Browser
authorization, protected-store permission prompts, actual playback commands and live
library/search/queue contents require that registration.

Windows audio hardware, installed Equalizer APO processing, extended sessions,
hot-unplug behavior and accessibility readers require testing on a normal Windows
or Linux desktop. Source tests and headless smoke tests do not establish those results.

Spotify terms and any permission required to distribute the combined application
remain a separate review; implementation success does not resolve that question.
