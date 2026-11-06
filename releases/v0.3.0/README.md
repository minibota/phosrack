# Phosrack 0.3.0

A compact modular music controller with a Winamp-inspired rack, local audio
visualization and an actual ten-band equalizer. Spotify requires your own public
Client ID and Premium account; authorization uses the official PKCE flow.

| Platform | Download | Requirement |
| --- | --- | --- |
| Windows x64 | [Installer](phosrack-0.3.0-windows-x64.exe) | Windows 11; unsigned development installer |
| Windows x64 | [Portable ZIP](phosrack-0.3.0-windows-x64.zip) | Extract the complete folder before running |
| Fedora 45 x64 | [RPM](phosrack-0.3.0-1.fc45.x86_64.rpm) | Compatible Qt/KDE/PipeWire runtime packages |
| Debian 13 x64 | [DEB](phosrack_0.3.0_amd64.deb) | Install with `sudo apt install ./phosrack_0.3.0_amd64.deb` |
| Linux x64 | [Flatpak](phosrack-0.3.0-x86_64.flatpak) | KDE Platform 6.10; a PipeWire session |

Check downloads against [SHA256SUMS](SHA256SUMS), then follow
[Getting started](../../GETTING_STARTED.md). Linux EQ creates an optional Phosrack EQ
output; choose it in system sound settings to process playback. Windows EQ requires
[Equalizer APO setup](../../docs/windows.md); its installation is separate and optional.

These exact artifacts were built from
[`6eddf71`](https://github.com/minibota/phosrack/commit/6eddf71adf888b52100137254f78dac6b7474b5e)
by the [Linux workflow](https://github.com/minibota/phosrack/actions/runs/42306148198)
and [native Windows workflow](https://github.com/minibota/phosrack/actions/runs/42306148235).
Linux and Windows source/deployment tests passed; physical Windows 11 audio devices,
installed APO processing and real Spotify authorization still need manual testing.
See the complete [validation record](../../docs/validation.md).

This is an independent GPL-3.0-or-later project, not affiliated with or endorsed by
Spotify, Winamp or Microsoft. Technical tests do not establish Spotify policy approval.
