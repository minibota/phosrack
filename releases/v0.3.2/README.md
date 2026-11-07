# Phosrack 0.3.2

Integrated Spotify playback with Qt WebEngine and the official Web Playback SDK,
personal Client ID configuration, and a faster fullscreen visualizer that retains
the phosphor appearance. The main window no longer has a maximize button.

Download installers from the [v0.3.2 release](https://github.com/minibota/phosrack/releases/tag/v0.3.2):

| Platform | Package |
| --- | --- |
| Fedora 45 x86_64 | [RPM](https://github.com/minibota/phosrack/releases/download/v0.3.2/phosrack-0.3.2-1.fc45.x86_64.rpm) |
| Debian 13 amd64 | [DEB](https://github.com/minibota/phosrack/releases/download/v0.3.2/phosrack_0.3.2_amd64.deb) |
| Linux x86_64 | [Flatpak](https://github.com/minibota/phosrack/releases/download/v0.3.2/phosrack-0.3.2-x86_64.flatpak) |
| Windows 11 x64 | [Installer](https://github.com/minibota/phosrack/releases/download/v0.3.2/phosrack-0.3.2-windows-x64.exe) · [Portable ZIP](https://github.com/minibota/phosrack/releases/download/v0.3.2/phosrack-0.3.2-windows-x64.zip) |

Large WebEngine bundles are release assets, not Git blobs. Checksums and build
provenance are recorded here after the release builds complete. Do not use a
historical version's checksums to verify these packages.

Spotify Premium, your own developer Client ID and compatible Widevine are required
for integrated playback. Widevine is not included. The external-player mode remains
available. See [installation and account setup](../../GETTING_STARTED.md),
[current limitations](../../README.md#limitations) and
[validation results](../../docs/validation-0.3.2.md).

To update an existing user installation of the Flatpak with the downloaded bundle:

    flatpak install --user --reinstall ./phosrack-0.3.2-x86_64.flatpak

Development packages are unsigned. Windows audio/DRM needs manual validation;
a passing build is not evidence that playback works on every Windows installation.
