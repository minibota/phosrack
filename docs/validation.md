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

Final RPM/DEB/Flatpak package installation checks will be recorded here after export.
The Linux packages include the native EQ worker. Flatpak uses the KDE runtime's public
PipeWire library and client modules; it needs no host-command execution or extra
manager permission.

Windows source includes WASAPI loopback, Windows Credential Manager and optional
Equalizer APO integration. A native MSVC workflow builds and tests the executable,
deploys its Qt/compiler dependencies and validates the deployed tree before creating
the portable ZIP and installer. Until that workflow completes, Windows runtime
validation is pending; no locally fabricated Windows executable is supplied.

## Remaining manual checks

No Spotify Client ID has been configured for the user's real account yet. Browser
authorization, protected-store permission prompts, actual playback commands and live
library/search/queue contents require that registration.

Windows audio hardware, installed Equalizer APO processing, extended sessions,
hot-unplug behavior and accessibility readers require testing on a normal Windows
or Linux desktop. Source tests and headless smoke tests do not establish those results.

Spotify terms and any permission required to distribute the combined application
remain a separate review; implementation success does not resolve that question.
