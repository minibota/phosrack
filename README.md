# Phosrack

[![Version 0.3.1](https://img.shields.io/badge/version-0.3.1-bce982)](https://github.com/minibota/phosrack/releases/tag/v0.3.1)
[![Linux build](https://github.com/minibota/phosrack/actions/workflows/build.yml/badge.svg?branch=main&label=Linux)](https://github.com/minibota/phosrack/actions/workflows/build.yml)
[![Windows build](https://github.com/minibota/phosrack/actions/workflows/windows.yml/badge.svg?branch=main&label=Windows)](https://github.com/minibota/phosrack/actions/workflows/windows.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)

**0.3.1 · QML + C++ desktop music rack** · GPL-3.0-or-later

A compact media-player rack inspired by the classic Winamp era, with modular panels,
a real local equalizer and an original fullscreen audio visualizer.

[Getting started](GETTING_STARTED.md) · [Installers](https://github.com/minibota/phosrack/releases/tag/v0.3.1) ·
[Build and packaging](docs/packaging.md) · [Windows](docs/windows.md) ·
[Validation](docs/validation-0.3.1.md) · [Issues](https://github.com/minibota/phosrack/issues)

## Screenshot

<img src="docs/images/phosrack-spotify.png" alt="Phosrack playing Spotify with the ten-band equalizer, live spectrum visualizer and playback queue" width="470">

Phosrack in English with Spotify playback, the equalizer, live spectrum visualization and the queue.

## Features

- Stackable, reorderable and collapsible player, visualizer, EQ and music panels.
- Artwork, playback position, volume, previous/next, shuffle and repeat.
- Search, queue and library views, including saved tracks, albums and playlists.
- Ten EQ bands, preamp, Flat/Rock/Pop/Jazz/Classical/Bass boost/Vocal presets and
  persistent custom gains.
- Orbit, Spectrum and Oscilloscope visualization, reduced motion, fullscreen mode,
  and an overlay for the current and next track.
- English, Spanish and French selected from the system language; English fallback.
- Connect your own Spotify developer application using a public Client ID and browser
  authorization with PKCE; no Client Secret is needed.
- Protected session storage: KWallet on Linux and Windows Credential Manager on Windows.

The application embeds its QML, icon and translation catalogs. It does not require
Plasma or a globally installed QML plugin. Spotify playback uses an existing player;
no private Spotify APIs, borrowed application credentials or librespot are used.

## Limitations

> [!IMPORTANT]
> **Spotify Premium is required for playback controls.** Phosrack controls Spotify
> playback; it does not stream music itself. For playback on this computer, keep the
> official Spotify desktop app running in the background or the
> [Spotify Web Player](https://open.spotify.com/) open in your browser.
> Start playback there first, then use Phosrack. You can also control another active
> Spotify Connect device, but its audio stays on that device.

Before installing, please be aware of these current limitations:

- **Spotify Free playback controls are not supported.** Supplying a Client ID
  does not unlock Premium features or bypass Spotify's
  [playback requirements](https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback).
- **Personal Spotify application setup is required.** Current source builds use
  each user's own Client ID. Spotify's development-mode limits still apply to that
  application, including up to five authorized users. Public downloads and browser
  sign-in alone do not grant API access. See [Spotify setup and scope](#spotify-setup-and-scope),
  including the instructions for existing v0.3.1 packages.
- **No offline playback or music downloads.** Spotify playback and API features
  require an internet connection; Phosrack cannot download songs or play them offline.
- **The visualizer and EQ work with local audio only.** They cannot visualize or
  process sound playing on a remote phone, speaker or other Spotify Connect device.
  The visualizer includes all audio on the monitored output, including other apps
  and notifications.
- **The equalizer needs additional setup.** Linux requires PipeWire and routing the
  player to the **Phosrack EQ** output. Windows requires a separate Equalizer APO
  installation configured for the playback device. See [Local audio](#local-audio).
- **Queue and library editing is limited.** You can view the queue and add tracks,
  but cannot remove or reorder queued items. Library views support browsing, not
  saving/removing library items or editing playlists.

## Install

Download the package for your platform from the
[v0.3.1 release](https://github.com/minibota/phosrack/releases/tag/v0.3.1).
Linux formats are RPM (Fedora 45 x86_64), DEB (Debian 13 amd64) and Flatpak.
The Windows workflow produces an x64 installer and portable ZIP using MSVC.

To update an existing user Flatpak installation with the downloaded bundle:

    flatpak install --user --reinstall ./phosrack-0.3.1-x86_64.flatpak

Follow [GETTING_STARTED.md](GETTING_STARTED.md) for installation, including the
per-command digest verification needed for unsigned development RPMs. Check
[validation notes](docs/validation-0.3.1.md) for completed build/runtime checks; a successful
Windows compilation does not by itself verify WASAPI or Equalizer APO on real hardware.

## Spotify setup and scope

**Use your own Spotify developer application for personal use.** Current source
builds do not include a shared Client ID. You configure your public application ID
once; Phosrack remembers it and uses PKCE to sign in to your own Spotify account.
A Client ID identifies an application, not your password. Never paste a Client Secret
or an access token into that field.

1. Sign in to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
   with your Premium account and create or select an application for your personal
   Phosrack setup.
2. Enable **Web API** and register this redirect URI exactly:
   `http://127.0.0.1:43821/callback`.
3. Copy the application's **Client ID**. Open Phosrack Settings, expand
   **Spotify application setup** if needed, and paste it into **Client ID (no Client Secret)**.
4. Click **Connect Spotify** and authorize the same Spotify account in your browser.
5. Open the official Spotify app or web player and start playback. Keep that player
   running while controlling it from Phosrack.

> [!NOTE]
> **Existing v0.3.1 installers still contain the earlier shared development ID.**
> Those packages have not been replaced by this source change. In v0.3.1, expand
> **Settings > Advanced Spotify settings**, replace the displayed Client ID with
> your own, then connect again. The redirect URI above is unchanged.

Spotify requires the owner of a development-mode application to retain Premium;
playback controls also require Premium for the listening account. Each new development
application allows up to five authenticated users. If you authorize another account,
add it in your application's **Users Management** allowlist first. Successful browser
login alone does not grant API access to an unlisted user.

Personal application setup avoids depending on Phosrack's shared tester list; it
does **not** remove Spotify's per-application limits or grant public access to your
application. Extended access requires Spotify approval under its partner eligibility
rules. See [Spotify's quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).
For optional build-time configuration of your own application ID, see the
[packaging guide](docs/packaging.md#spotify-application-configuration).

The requested permissions cover playback, saved library and playlists. Older 0.1
sessions need reauthorization for library permissions. Development-mode search is
limited to ten results per content type per page; playlist contents can be read only
for owned/collaborative playlists. The application handles restrictions, empty playback,
token refresh and request throttling.

This project is independent of Spotify and is not presented as Spotify-approved.
Use of the official API does not establish compliance of every combined feature.
The developer policy's visual-media and core-experience provisions still need to be
resolved before representing the application as an approved public Spotify client.
Publishing source does not grant additional API access or permission.

## Build from source

Qt 6.5 or later and a C++17 compiler are required.

Fedora 45 build dependencies:

    sudo dnf install gcc-c++ cmake ninja-build pkgconf-pkg-config qt6-qtbase-devel qt6-qtdeclarative-devel kf6-kwallet-devel extra-cmake-modules pipewire-devel pipewire-utils

Debian 13 build dependencies:

    sudo apt install build-essential cmake ninja-build pkg-config qt6-base-dev qt6-declarative-dev libkf6wallet-dev extra-cmake-modules libpipewire-0.3-dev qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtqml-workerscript qml6-module-qttest pipewire-bin

Then build and run:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPHOSRACK_SPOTIFY_CLIENT_ID=
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ./build/phosrack

For a user-local Linux installation, run:

    bash scripts/install-user.sh

Windows uses MSVC 2022 and the Qt Windows SDK, without KWallet or Linux PipeWire.
See [docs/windows.md](docs/windows.md). Packages are built with their target
distribution/toolchain; Fedora binaries are not converted into Debian packages.

## Local audio

On Linux, the visualizer follows the actual default PipeWire output. A separate
phosrack-eq-worker owns a virtual Audio/Sink and a playback stream pinned to the selected
physical output. Its local ten-band biquad DSP accepts live gains over private stdin
without restarting the stream or requiring a PipeWire manager permission.

Enable EQ, then choose **Phosrack EQ** as the player output in the system sound settings.
Phosrack does not silently change the default output or move other applications.
Disabling EQ or closing the application removes the temporary sink. A lower preamp
provides headroom when bands are boosted.

On Windows, visualization uses WASAPI loopback of the playback device. EQ integrates
with an optional, separately installed Equalizer APO driver. Configure the playback
device in Equalizer APO and choose its configuration folder in Phosrack Settings.
Phosrack preserves unrelated configuration, creates a backup and writes its own
filter include. The driver itself is not bundled.

Visual analysis includes all local audio on the selected output, potentially including
other apps and notifications. It does not receive sound from remote Spotify devices.
Hiding/folding the visualizer or minimizing its last visible window stops monitoring.
Raw samples are processed in memory, without audio files or uploads.

Fullscreen metadata comes from actual Spotify playback and queue data; unavailable
information is shown explicitly. Escape or the close button returns to the rack.

## Development and license

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md),
[architecture.md](docs/architecture.md) and [LICENSE](LICENSE).
Artwork and effects are original; no Winamp or Windows Media Player assets are bundled.
Phosrack is independent of Spotify, Winamp, KDE and Microsoft.

The repository is https://github.com/minibota/phosrack. Installable artifacts are kept
under releases/v0.3.1/. Fully detachable rack panels remain future work.

## Primary references

- https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow
- https://developer.spotify.com/documentation/web-api/concepts/quota-modes
- https://developer.spotify.com/documentation/web-api/tutorials/february-2026-migration-guide
- https://developer.spotify.com/policy
- https://docs.pipewire.org/
- https://doc.qt.io/qt-6.8/windows-deployment.html
- https://docs.flatpak.org/en/latest/sandbox-permissions.html
