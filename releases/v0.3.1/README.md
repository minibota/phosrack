# Phosrack 0.3.1

Open Settings and click **Connect Spotify**, then authorize your own Spotify account
in the browser. The release includes the project's public application ID. Manual
Client ID configuration is under **Advanced Spotify settings** for developers and forks.

Spotify's development-mode limits still apply: the application owner needs Premium
and new applications permit up to five authenticated, allowlisted users. Downloading
Phosrack does not add an account to that allowlist or grant access to the developer's
account. Playback requires Premium and an active Spotify device. See
[Spotify's quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

## Planned packages

The native workflows produce these five files from one source revision:

| Platform | File |
| --- | --- |
| Windows 11 x64 | `phosrack-0.3.1-windows-x64.exe` |
| Windows x64 portable | `phosrack-0.3.1-windows-x64.zip` |
| Fedora 45 x64 | `phosrack-0.3.1-1.fc45.x86_64.rpm` |
| Debian 13 amd64 | `phosrack_0.3.1_amd64.deb` |
| Linux x64 Flatpak | `phosrack-0.3.1-x86_64.flatpak` |

Build and validation results will be recorded after the actual 0.3.1 workflows pass.
No 0.3.0 test result is presented as a 0.3.1 package check. Follow
[Getting started](../../GETTING_STARTED.md) and the
[0.3.1 validation notes](../../docs/validation-0.3.1.md).

The local equalizer and visualizer retain their 0.3.0 platform requirements:
PipeWire on Linux and optional, separately installed Equalizer APO for Windows EQ.
See [Windows setup](../../docs/windows.md). Packages are unsigned development builds.

This independent GPL-3.0-or-later project is not affiliated with or endorsed by
Spotify, Winamp or Microsoft. Embedding a public Client ID and passing technical
tests do not establish Spotify approval or remove its distribution/API restrictions.
