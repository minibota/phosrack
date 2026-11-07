# Phosrack

[![Version 0.3.2](https://img.shields.io/badge/version-0.3.2-bce982)](https://github.com/minibota/phosrack/releases/tag/v0.3.2)
[![Linux build](https://github.com/minibota/phosrack/actions/workflows/build.yml/badge.svg?branch=main&label=Linux)](https://github.com/minibota/phosrack/actions/workflows/build.yml)
[![Windows build](https://github.com/minibota/phosrack/actions/workflows/windows.yml/badge.svg?branch=main&label=Windows)](https://github.com/minibota/phosrack/actions/workflows/windows.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)

**0.3.2 · QML + C++ desktop music rack** · GPL-3.0-or-later

A compact music player inspired by the classic Winamp era. Arrange its panels your
way, shape the sound with the equalizer, and enjoy glowing visuals that react to
your music. Listen inside Phosrack or use it to control another Spotify player.

[Getting started](GETTING_STARTED.md) · [Installers](https://github.com/minibota/phosrack/releases/tag/v0.3.2) ·
[Build and packaging](docs/packaging.md) · [Windows](docs/windows.md) ·
[Validation](docs/validation-0.3.2.md) · [Issues](https://github.com/minibota/phosrack/issues)

## Screenshot

<img src="docs/images/phosrack-spotify.png" alt="Phosrack playing Spotify with the ten-band equalizer, live spectrum visualizer and playback queue" width="470">

Phosrack in English with Spotify playback, the equalizer, live spectrum visualization and the queue.

## Features

- Stack, reorder or fold the player, equalizer, visualizer and music panels.
- Listen to Spotify inside Phosrack, or control Spotify on another player or device.
- Album artwork, playback progress, volume, previous/next, shuffle and repeat.
- Search for music, see what's coming next, and browse your saved tracks, albums
  and playlists.
- A ten-band equalizer with Flat, Rock, Pop, Jazz, Classical, Bass boost and Vocal
  presets. Adjust the sound yourself and keep your custom settings.
- Three visual styles: Orbit, Spectrum and Oscilloscope. Go fullscreen to see the
  current and next song, or choose reduced motion for calmer effects.
- English, Spanish and French, following your computer's language. Other languages
  use English.
- Sign in through your browser after a one-time Spotify setup. Your saved sign-in
  uses your system's protected storage: KWallet on Linux or Credential Manager on Windows.

Phosrack is a standalone desktop app. You do not need KDE Plasma to use it.

## Limitations

> [!IMPORTANT]
> Phosrack can play Spotify audio inside the app or control another Spotify player.
> Choose the playback mode in Settings.
>
> **Both modes need Spotify Premium, an internet connection and your own Client ID.**
> A Client ID is a code you get from Spotify during the one-time setup below.
> It identifies your personal application; it is not your password.
>
> **Play in Phosrack** (the default) also needs **Widevine**, a component used to
> play protected music, plus compatible audio support on your computer. Widevine
> is not included with Phosrack. If Chrome is already installed, Phosrack looks
> for a copy it can use.
> **Control another Spotify player** needs an active Spotify app, web player or
> Spotify Connect device. The music plays on whichever player you choose.

A few things to know before installing:

- **Spotify Free is not supported for playback.** Spotify requires Premium for
  [playing inside the app](https://developer.spotify.com/documentation/web-playback-sdk/tutorials/getting-started)
  and [controlling playback](https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback).
  A Client ID does not change your subscription.
- **A one-time Spotify setup is required.** You will create your own Client ID.
  Spotify's limits still apply: a new developer application can have up to five
  approved users, and its owner must keep Premium. Spotify also limits how often
  applications can contact its service. Signing in alone does not approve an account
  for your application. See [Spotify setup](#spotify-setup-and-scope) and
  [Spotify's access rules](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).
- **Playback inside the app can need extra setup.** Finding Widevine does not
  guarantee it will work with your system's audio support. You can select an
  existing copy in Settings and restart Phosrack. Linux sound and pause/resume
  have been tested. Playback inside the Windows app is experimental: its included
  audio support has limitations, and listening tests are still pending. Use the
  other-player mode if music will not start. A complete Flatpak sign-in/listening
  test is also pending. See [what has been tested](docs/validation-0.3.2.md) and
  [Qt's technical requirements](https://doc.qt.io/qt-6.8/qtwebengine-features.html#html5-drm).
- **No offline listening or downloads.** Phosrack needs an internet connection
  to play Spotify music and browse your account.
- **The visualizer and equalizer use sound from this computer.** They cannot react
  to or change music playing on another Spotify Connect device, such as a phone
  or smart speaker. The visualizer hears
  everything on the selected sound output, including other apps and notifications.
- **The equalizer needs a little setup.** On Linux, choose **Phosrack EQ** as the
  sound output for the app playing your music. On Windows, install Equalizer APO
  separately and enable it for your speakers or headphones. See [Local audio](#local-audio).
- **Some music management features are not available yet.** You can see the queue
  and add songs, but cannot remove or rearrange them. You can browse your library,
  but cannot save/remove library items or edit playlists. Spotify only lets these
  personal developer applications [read playlist contents](https://developer.spotify.com/documentation/web-api/tutorials/february-2026-migration-guide#playlist-endpoint-renames)
  for playlists you own or help edit.

## Install

Download the file for your computer from the
[v0.3.2 release](https://github.com/minibota/phosrack/releases/tag/v0.3.2):

- **Windows 11:** choose the `.exe` installer. The `.zip` is a portable version:
  extract it into a folder and open `phosrack.exe`.
- **Linux:** choose Flatpak, or the package for your distribution: RPM for Fedora 45
  or DEB for Debian 13. These downloads are for 64-bit Intel/AMD computers.

Already using the Flatpak? Run this command in the folder containing your download:

    flatpak install --user --reinstall ./phosrack-0.3.2-x86_64.flatpak

The [getting started guide](GETTING_STARTED.md) has the installation commands,
including the extra step Fedora needs for these unsigned packages. The
[testing notes](docs/validation-0.3.2.md) explain which features have been checked
on each platform.

## Spotify setup and scope

**This is a one-time setup for your own Spotify account.** You will create an entry
in Spotify's developer dashboard and copy its **Client ID** into Phosrack. You do
not need to write code, and Phosrack remembers the ID for next time.

1. Open the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
   and sign in with your Premium account. Choose **Create app**, or open an
   application you already use for your personal Phosrack setup.
2. In that application's settings, select **Web API** and **Web Playback SDK**.
   In **Redirect URIs**, add this exact address, then save:
   `http://127.0.0.1:43821/callback`.
   This address lets the browser return your sign-in to Phosrack on your computer.
3. Copy the application's **Client ID**. In Phosrack, open **Settings > Spotify
   application setup** and paste it into **Client ID (no Client Secret)**.
   Use the Client ID only; never paste a Client Secret, password or access token.
4. Click **Connect Spotify**. Your browser will open Spotify's sign-in and permission
   screen. Approve access with the same account, then return to Phosrack.
5. In **Settings > Playback**, choose how you want to listen:
   **Play in Phosrack** or **Control another Spotify player**.

For **Play in Phosrack**, check the player status in Settings. Phosrack tries to find
Widevine in an existing Chrome installation. If it cannot find a compatible copy,
**Choose Widevine module…** lets you select one from an existing trusted installation;
restart Phosrack after changing it. The [setup guide](GETTING_STARTED.md#playback-and-widevine)
explains which file to choose. Once the player is ready, play a song from **Search**
or **Library**. If you are already listening elsewhere, the Play button can bring
that session into Phosrack.

For **Control another Spotify player**, start the Spotify app,
[Spotify Web Player](https://open.spotify.com/) or another Spotify Connect device,
then select it in Settings. Keep that player running; it produces the sound.
This mode is also useful if playback inside Phosrack is unavailable on your computer.

> [!NOTE]
> **Upgrading from v0.3.1:** if you used the shared application, enter your own
> Client ID. A personal ID you saved earlier is kept. Enable **Web Playback SDK**
> in its Spotify settings and reconnect when asked to approve the new permissions.
> The redirect address above has not changed.

To let another account use your application, add it under **Users Management** in
the Spotify dashboard first. Both the application owner and the person listening
need Premium for this setup. New applications can have up to five approved users;
a successful browser sign-in does not replace that approval. Your own Client ID
gives you a separate personal setup, rather than removing Spotify's limits. Wider
public access requires Spotify's approval under its
[partner access rules](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

Spotify asks for permission to control playback, read your library and playlists,
and read the account details needed by its player. If you connected an older version
of Phosrack, you may be asked to connect again to approve the added permissions.
Search results arrive in pages of up to ten items of each type. Some playlists and
playback options may be unavailable because of Spotify's account or device restrictions.

Phosrack is an independent project and is **not approved by Spotify**. Using Spotify's
official tools or publishing the source does not automatically approve every feature
or grant wider account access. In particular, the visualizer and the combined player
experience still need review against [Spotify's developer policy](https://developer.spotify.com/policy)
before the project can claim such approval.

## Build from source

Qt 6.5 or later, including WebEngine Core, WebChannel and Widgets, and a C++17
compiler are required. Widevine is a separate runtime dependency for integrated playback.

Fedora 45 build dependencies:

    sudo dnf install gcc-c++ cmake ninja-build pkgconf-pkg-config qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwebengine-devel qt6-qtwebchannel-devel kf6-kwallet-devel extra-cmake-modules pipewire-devel pipewire-utils

Debian 13 build dependencies:

    sudo apt install build-essential cmake ninja-build pkg-config qt6-base-dev qt6-declarative-dev qt6-webengine-dev qt6-webchannel-dev libkf6wallet-dev extra-cmake-modules libpipewire-0.3-dev qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-window qml6-module-qtqml-workerscript qml6-module-qttest pipewire-bin

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
For integrated playback, route Phosrack's audio stream to that output; in external
mode, route the Spotify app or browser that is producing the sound.
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
Integrated music playback continues while the rack is minimized.
Raw samples are processed in memory, without audio files or uploads.

Fullscreen metadata comes from actual Spotify playback and queue data; unavailable
information is shown explicitly. Escape or the close button returns to the rack.

## Development and license

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md),
[architecture.md](docs/architecture.md) and [LICENSE](LICENSE).
Artwork and effects are original; no Winamp or Windows Media Player assets are bundled.
Phosrack is independent of Spotify, Winamp, KDE and Microsoft.

The repository is https://github.com/minibota/phosrack. Installers are attached to
GitHub Releases; versioned release folders contain the download index and checksums.
Large packages are kept out of Git history. Fully detachable rack panels remain future work.

## Primary references

- https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow
- https://developer.spotify.com/documentation/web-playback-sdk
- https://developer.spotify.com/documentation/web-api/concepts/quota-modes
- https://developer.spotify.com/documentation/web-api/tutorials/february-2026-migration-guide
- https://developer.spotify.com/policy
- https://docs.pipewire.org/
- https://doc.qt.io/qt-6.8/windows-deployment.html
- https://doc.qt.io/qt-6.8/qtwebengine-features.html#html5-drm
- https://docs.flatpak.org/en/latest/sandbox-permissions.html
