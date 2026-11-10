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

The following paths are reserved for the [v0.3.3 release](https://github.com/minibota/phosrack/releases/tag/v0.3.3).
Publication and artifact verification are pending; a link here is not evidence that
an installer has been built or tested.

| Platform | Package |
| --- | --- |
| Fedora 45 x86_64 | [RPM](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-1.fc45.x86_64.rpm) |
| Debian 13 amd64 | [DEB](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack_0.3.3_amd64.deb) |
| Linux x86_64 | [Flatpak](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-x86_64.flatpak) |
| Windows 11 x64 | [Installer](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-windows-x64.exe) · [Portable ZIP](https://github.com/minibota/phosrack/releases/download/v0.3.3/phosrack-0.3.3-windows-x64.zip) |

Checksums and component notices will be added after the final packages are built.
Large WebEngine bundles are published as release assets, not Git blobs.

## Build provenance and validation

Pending final source commit, workflow runs, package hashes and installed-runtime
checks. The [v0.3.3 validation record](../../docs/validation-0.3.3.md) distinguishes
planned checks from completed results. Previous release results are not proof that
these new packages pass.

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
