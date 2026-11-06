# Phosrack 0.3.1

Open Settings and click **Connect Spotify**, then authorize your own Spotify account
in the browser. The release includes the project's public application ID. Manual
Client ID configuration is under **Advanced Spotify settings** for developers and forks.

Spotify's development-mode limits still apply: the application owner needs Premium
and new applications permit up to five authenticated, allowlisted users. Downloading
Phosrack does not add an account to that allowlist or grant access to the developer's
account. Playback requires Premium and an active Spotify device. See
[Spotify's quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

## Downloads

Download from the [GitHub release](https://github.com/minibota/phosrack/releases/tag/v0.3.1)
or use the versioned files below. Check them against [SHA256SUMS](SHA256SUMS).

| Platform | File |
| --- | --- |
| Windows 11 x64 | [Installer](phosrack-0.3.1-windows-x64.exe) |
| Windows x64 portable | [ZIP](phosrack-0.3.1-windows-x64.zip) |
| Fedora 45 x64 | [RPM](phosrack-0.3.1-1.fc45.x86_64.rpm) |
| Debian 13 amd64 | [DEB](phosrack_0.3.1_amd64.deb) |
| Linux x64 Flatpak | [Bundle](phosrack-0.3.1-x86_64.flatpak) |

All packages use application source
[`8023913`](https://github.com/minibota/phosrack/commit/80239131e903c1450c2b0118f7c9d05fd12dcd1d).
RPM and DEB are exact native CI artifacts, with 17/17 CTest checks passing for each.
Windows is the exact native CI deployment, with 16/16 checks and clean deployment
smoke tests passing. The Flatpak was built locally from a clean archive of the same
commit using KDE SDK 6.10, then installed and smoke-tested in an isolated profile;
its CI job was still provisioning build tools when the release was prepared.
See [Getting started](../../GETTING_STARTED.md) and the
[validation record](../../docs/validation-0.3.1.md) for provenance and manual checks.

The local equalizer and visualizer retain their 0.3.0 platform requirements:
PipeWire on Linux and optional, separately installed Equalizer APO for Windows EQ.
See [Windows setup](../../docs/windows.md). Packages are unsigned development builds.

This independent GPL-3.0-or-later project is not affiliated with or endorsed by
Spotify, Winamp or Microsoft. Embedding a public Client ID and passing technical
tests do not establish Spotify approval or remove its distribution/API restrictions.
