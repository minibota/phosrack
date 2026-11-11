# Phosrack 0.3.5

The stereo meter follows left and right channels separately. Notes uses the full
panel. The new phosphor lightning icon, liked-songs control, title marquee and
last-played recovery update the rack while preserving the existing layout.
The equalizer preamp starts at 0 dB and stays where you set it across presets.

## Downloads

| Platform | Package |
| --- | --- |
| Fedora 45 x86_64 | [RPM](https://github.com/minibota/phosrack/releases/download/v0.3.5/phosrack-0.3.5-1.fc45.x86_64.rpm) |
| Debian 13 amd64 | [DEB](https://github.com/minibota/phosrack/releases/download/v0.3.5/phosrack_0.3.5_amd64.deb) |
| Linux x86_64 | [Flatpak](https://github.com/minibota/phosrack/releases/download/v0.3.5/phosrack-0.3.5-x86_64.flatpak) |
| Windows 11 x64 | [Installer](https://github.com/minibota/phosrack/releases/download/v0.3.5/phosrack-0.3.5-windows-x64.exe) · [Portable ZIP](https://github.com/minibota/phosrack/releases/download/v0.3.5/phosrack-0.3.5-windows-x64.zip) |

Verify the files against [SHA256SUMS.txt](SHA256SUMS.txt). See the
[third-party notices](THIRD_PARTY.md) and [installation guide](../../GETTING_STARTED.md).

## Build provenance

All five packages use application source
[`4c008b7e40bda9d6170af6bec47f0d3926071266`](https://github.com/minibota/phosrack/tree/4c008b7e40bda9d6170af6bec47f0d3926071266).
The RPM and DEB use [Linux CI](https://github.com/minibota/phosrack/actions/runs/43074515443),
the EXE and ZIP use [Windows CI](https://github.com/minibota/phosrack/actions/runs/43074515540),
and the Flatpak was built locally with the KDE 6.10 runtime.
See the [validation record](../../docs/validation-0.3.5.md) for the live audio,
packaging and runtime checks.
