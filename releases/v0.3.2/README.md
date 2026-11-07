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

Large WebEngine bundles are release assets, not Git blobs. RPM and DEB files are
also included in this directory. Verify downloads with [SHA256SUMS.txt](SHA256SUMS.txt).

## Build provenance

Linux packages use application source
[`a9e9b1359e1789dd2868c9b88c9c5e2e367217a8`](https://github.com/minibota/phosrack/tree/a9e9b1359e1789dd2868c9b88c9c5e2e367217a8).
Windows packages use [`4f048bcf4218d863f002bfd45d6eeb7c30b495f4`](https://github.com/minibota/phosrack/tree/4f048bcf4218d863f002bfd45d6eeb7c30b495f4),
which adds the Windows installation-root correction and final installer checks.
The music-player source is identical between these commits; the Linux installation
prefix remains `/usr`. Later commits add documentation, validation and release assets.

- [Linux CI](https://github.com/minibota/phosrack/actions/runs/42321879284) produced
  the RPM and DEB; their runtime checks passed on Fedora and in a clean Debian container.
- [Windows CI](https://github.com/minibota/phosrack/actions/runs/42323150248) produced
  the installer and portable archive, including clean deployment checks.
- The published Flatpak was built locally from the same source with KDE / Qt WebEngine
  BaseApp 6.10 and validated after installation in an isolated workspace. Its application
  commit is `800b80db2718922c5765b27e2371a909da8c96a9c2ef1238a3ac2a186b972d6b`.

Component/source information: [THIRD_PARTY.md](THIRD_PARTY.md).

Spotify Premium, your own developer Client ID and compatible Widevine are required
for integrated playback. Widevine is not included. The external-player mode remains
available. See [installation and account setup](../../GETTING_STARTED.md),
[current limitations](../../README.md#limitations) and
[validation results](../../docs/validation-0.3.2.md).

To update an existing user installation of the Flatpak with the downloaded bundle:

    flatpak install --user --reinstall ./phosrack-0.3.2-x86_64.flatpak

Development packages are unsigned. Native Linux audio and pause/resume were confirmed
with a live account. Flatpak passed installed UI and audio-format/DRM checks; authenticated
listening remains untested in the sandbox. Windows integrated playback is experimental:
its Qt build supports Opus/Vorbis but lacks AAC, and authenticated audio remains untested.
Use external-player mode if the local player cannot initialize.
