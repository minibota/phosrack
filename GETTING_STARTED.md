# Getting started with Phosrack 0.3.0

Phosrack is a QML + C++ desktop music rack with playback controls, search, queue,
library, a local equalizer and fullscreen visualization. The interface follows the
system language (English, Spanish or French); unsupported languages use English.

## Install a development package

Installers are stored under [releases/v0.3.0](releases/v0.3.0/). Choose the package
for your operating system and CPU architecture. See
[docs/validation.md](docs/validation.md) for the exact validation status.

- Fedora 45 x86_64: install the RPM after its runtime dependencies.
- Debian 13 amd64: install the DEB with your package manager.
- Linux Flatpak: install the local Flatpak bundle.
- Windows 11 x64: use the Windows installer or extract the portable ZIP and open
  phosrack.exe. Windows packages are produced by the native MSVC workflow.

Local development RPMs are unsigned. On Fedora 45, install the signed runtime
dependencies, then use a per-command digest check without changing global policy:

    sudo dnf install qt6-qtdeclarative qt6-qtsvg qt6-qtwayland kf6-kwallet pipewire pipewire-libs pipewire-utils
    sudo rpm --define '_pkgverify_level digest' -Uvh ./phosrack-0.3.0-1.fc45.x86_64.rpm

On Debian 13:

    sudo apt install ./phosrack_0.3.0_amd64.deb

For Flatpak:

    flatpak install --user ./phosrack-0.3.0-x86_64.flatpak

See [docs/packaging.md](docs/packaging.md) for build instructions and dependencies.

## Connect Spotify

1. Create an application in the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard).
2. Enable Web API and register exactly http://127.0.0.1:43821/callback.
3. Open Settings, paste the Client ID and click Connect Spotify.
4. Authorize the application in your browser. A Client Secret is not needed.
5. Open Spotify and start playback on a device.

Session credentials use the operating system's protected storage when available.
Disconnect removes the saved token. Premium and Spotify's API restrictions still
apply. This build does not use private Spotify interfaces or another project's ID.

## Equalizer

Choose a preset or edit the ten bands and preamp. Boosting bands can clip loud audio;
reduce the preamp level to leave headroom.

On Linux, enable EQ and select **Phosrack EQ** as the output for your player in the
system sound settings. The effect works locally without a Spotify connection.
Phosrack creates a temporary processing output and does not silently reroute apps
or change the system default. Disabling EQ or closing Phosrack removes that output.

Windows uses the optional Equalizer APO integration described in the application
and [Windows packaging guide](docs/windows.md). Equalizer APO must be installed and
configured for your playback device separately; it is not bundled with Phosrack.

## Visualizer and fullscreen

The visualizer follows the default local output while visible. Output override and
reduced-motion settings are available in Settings. It includes audio from other
applications on that output, and does not receive sound from remote Spotify devices.

Click the fullscreen icon to open the large visualizer. Its overlay shows the
current track and the next item in the Spotify queue. Escape or the close button
returns to the rack. Unavailable song information is shown honestly when Spotify
is disconnected or the queue is empty.

## Project status

Source is licensed GPL-3.0-or-later. Technical implementation does not imply Spotify
approval: distribution of the combined application remains subject to Spotify's
terms and any required permission. See README and the validation notes before
publishing a release.
