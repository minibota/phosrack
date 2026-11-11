# Phosrack 0.3.4

The taller visualizer adds an 8 × 6 phosphor grid and a display of predominant
notes and probable chords. The equalizer adds four presets and can run alongside
Spotify playback. Fullscreen keeps its header visible and shows an upcoming
track only when one is queued.

## Downloads

| Platform | Package |
| --- | --- |
| Fedora 45 x86_64 | [RPM](https://github.com/minibota/phosrack/releases/download/v0.3.4/phosrack-0.3.4-1.fc45.x86_64.rpm) |
| Debian 13 amd64 | [DEB](https://github.com/minibota/phosrack/releases/download/v0.3.4/phosrack_0.3.4_amd64.deb) |
| Linux x86_64 | [Flatpak](https://github.com/minibota/phosrack/releases/download/v0.3.4/phosrack-0.3.4-x86_64.flatpak) |
| Windows 11 x64 | [Installer](https://github.com/minibota/phosrack/releases/download/v0.3.4/phosrack-0.3.4-windows-x64.exe) · [Portable ZIP](https://github.com/minibota/phosrack/releases/download/v0.3.4/phosrack-0.3.4-windows-x64.zip) |

Verify the files against [SHA256SUMS.txt](SHA256SUMS.txt). See the
[third-party notices](THIRD_PARTY.md) and [installation guide](../../GETTING_STARTED.md).

## Build provenance

All five packages use application source
[`f7a262ec012bcbb7a1a57d0627630fe3d878c1f6`](https://github.com/minibota/phosrack/tree/f7a262ec012bcbb7a1a57d0627630fe3d878c1f6).
The RPM and DEB use [Linux CI](https://github.com/minibota/phosrack/actions/runs/43060633654),
the EXE and ZIP use [Windows CI](https://github.com/minibota/phosrack/actions/runs/43060633746),
and the Flatpak was built locally with the KDE 6.10 runtime.
See the [validation record](../../docs/validation-0.3.4.md) for the live audio,
packaging and runtime checks.
