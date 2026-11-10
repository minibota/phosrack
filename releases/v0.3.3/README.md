# Phosrack 0.3.3

Restore saved Spotify sessions on startup, receive a clear prompt when browser
sign-in is needed, and return through a Phosrack confirmation page in English,
Spanish or French. Background requests are reduced, duplicate queue refreshes are
coalesced, and Spotify's API wait is preserved across restart and disconnect.

Spotify mode now uses ambient phosphor visuals without system-audio analysis or
Phosrack EQ. The separate Local audio tools mode is for non-Spotify sound and
suspends the Spotify session while keeping the saved sign-in. See the
[policy review](../../docs/spotify-policy.md) for the reasoning and remaining questions.

## Downloads

Download installers from the [v0.3.3 release](https://github.com/minibota/phosrack/releases/tag/v0.3.3).

| Platform | Package |
| --- | --- |
| Fedora 45 x86_64 | [RPM](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-1.fc45.x86_64.rpm) |
| Debian 13 amd64 | [DEB](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack_0.3.3_amd64.deb) |
| Linux x86_64 | [Flatpak](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-x86_64.flatpak) |
| Windows 11 x64 | [Installer](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-windows-x64.exe) · [Portable ZIP](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-windows-x64.zip) |

Verify downloads with [SHA256SUMS.txt](SHA256SUMS.txt). See [THIRD_PARTY.md](THIRD_PARTY.md)
for component sources and notices. RPM and DEB files are also in this directory;
large WebEngine bundles are published as release assets, not Git blobs.

## Build provenance and validation

RPM, Windows and Flatpak use application source
[`c6c702091f8b3ab3c7fdd7a3aadf2d48732be004`](https://github.com/minibota/phosrack/tree/c6c702091f8b3ab3c7fdd7a3aadf2d48732be004).
The DEB uses [`80091d588cfd10cbbda9874bfadb447195adf36a`](https://github.com/minibota/phosrack/tree/80091d588cfd10cbbda9874bfadb447195adf36a),
which adds the required Debian SVG image plugin; application code is unchanged.

- [RPM CI](https://github.com/minibota/phosrack/actions/runs/42842329313): 18/18 tests; payload digests and extracted UI smoke checks passed on Fedora.
- [DEB CI](https://github.com/minibota/phosrack/actions/runs/42843464979): 18/18 tests; apt installation and UI checks passed in a clean Debian runtime container.
- [Windows CI](https://github.com/minibota/phosrack/actions/runs/42842329398): 18/18 tests, portable ZIP verification, installed NSIS UI and Start-menu shortcut checks, and uninstall passed.
- Flatpak: built locally from the exact application source, then installed and checked in an isolated Flatpak root with KDE / Qt WebEngine BaseApp 6.10. App commit: `32067e751b6c7b818d7e87d4e61f08b2cf6940248be1fb7ef217477004672072`.

See the [v0.3.3 validation record](../../docs/validation-0.3.3.md) for scope and limitations.
Live Spotify sign-in and listening were not repeated during the existing cooldown.

Spotify Premium, a personal developer Client ID and compatible Widevine are needed
for integrated playback. Widevine is not included. External-player mode remains
available. Development quotas remain under Spotify's control and are shared across
apps owned by the same developer account; another Client ID is not a reset.

Read [installation and account setup](../../GETTING_STARTED.md),
[current limitations](../../README.md#limitations), [privacy](../../PRIVACY.md) and
[integration terms](../../TERMS.md) before connecting your account.

To update an existing user installation of Flatpak after downloading the bundle:

    flatpak install --user --reinstall ./phosrack-0.3.3-x86_64.flatpak

Development packages are unsigned. Windows integrated audio remains experimental;
authenticated listening and platform-specific hardware checks must be reported
separately from build, codec and DRM initialization results.
