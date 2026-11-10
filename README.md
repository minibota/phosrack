# Phosrack

[![Version 0.3.3](https://img.shields.io/badge/version-0.3.3-bce982)](https://github.com/minibota/phosrack/releases/tag/v0.3.3)
[![Linux build](https://github.com/minibota/phosrack/actions/workflows/build.yml/badge.svg?branch=main&label=Linux)](https://github.com/minibota/phosrack/actions/workflows/build.yml)
[![Windows build](https://github.com/minibota/phosrack/actions/workflows/windows.yml/badge.svg?branch=main&label=Windows)](https://github.com/minibota/phosrack/actions/workflows/windows.yml)
[![License: GPL-3.0-or-later](https://img.shields.io/badge/license-GPL--3.0--or--later-blue)](LICENSE)

**0.3.3 · QML + C++ desktop music rack** · GPL-3.0-or-later

A compact music player inspired by the classic Winamp era. Arrange its panels your
way and enjoy its green phosphor glow. Listen inside Phosrack or use it to control
another Spotify player. Separate local audio tools provide an equalizer and reactive
visuals for non-Spotify sound.

[Getting started](GETTING_STARTED.md) · [Installers](https://github.com/minibota/phosrack/releases/tag/v0.3.3) ·
[Build and packaging](docs/packaging.md) · [Windows](docs/windows.md) ·
[Validation](docs/validation-0.3.3.md) · [Privacy](PRIVACY.md) · [Terms](TERMS.md) · [Issues](https://github.com/minibota/phosrack/issues)

## Screenshot

<img src="docs/images/phosrack-spotify.png" alt="Phosrack playing Spotify with the ten-band equalizer, live spectrum visualizer and playback queue" width="470">

The Phosrack rack in English. This screenshot shows an earlier version; v0.3.3
separates Spotify playback from the local equalizer and audio-reactive visuals.

## Features

- Stack, reorder or fold the player, equalizer, visualizer and music panels.
- Listen to Spotify inside Phosrack, or control Spotify on another player or device.
- Album artwork, playback progress, volume, previous/next, shuffle and repeat.
- Search for music, see what's coming next, and browse your saved tracks, albums
  and playlists.
- Three phosphor visual styles and fullscreen, with reduced motion for calmer
  effects. Spotify mode uses ambient animation and can show the current and next song.
- Separate **Local audio tools** for non-Spotify sound: a reactive visualizer and
  ten-band equalizer with Flat, Rock, Pop, Jazz, Classical, Bass boost and Vocal
  presets. Keep your own custom settings.
- English, Spanish and French, following your computer's language. Other languages
  use English.
- Sign in through your browser after a one-time Spotify setup. Phosrack restores
  your saved session on opening and tells you when browser authorization is needed.
  Saved credentials use KWallet on Linux or Credential Manager on Windows.

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
  test is also pending. See [what has been tested](docs/validation-0.3.3.md) and
  [Qt's technical requirements](https://doc.qt.io/qt-6.8/qtwebengine-features.html#html5-drm).
- **No offline listening or downloads.** Phosrack needs an internet connection
  to play Spotify music and browse your account.
- **Spotify playback uses ambient visuals and no Phosrack EQ.** Audio-reactive
  visuals and equalization are separate tools for non-Spotify audio. Opening them
  suspends Spotify in Phosrack. They hear the whole selected computer output,
  so stop Spotify in other apps too. See [Local audio](#local-audio).
- **Spotify can temporarily pause access.** Phosrack waits for the time Spotify
  requests, even after a restart. Development quotas are shared by applications
  owned by the same Spotify developer account. Another Client ID is not a reset.
  See [limits and project scope](docs/spotify-policy.md).
- **The local equalizer needs a little setup.** Linux uses a temporary **Phosrack EQ**
  output; Windows needs Equalizer APO installed separately for your speakers or
  headphones. These tools are for audio you are entitled to process.
- **Some music management features are not available yet.** You can see the queue
  and add songs, but cannot remove or rearrange them. You can browse your library,
  but cannot save/remove library items or edit playlists. Spotify only lets these
  personal developer applications [read playlist contents](https://developer.spotify.com/documentation/web-api/tutorials/february-2026-migration-guide#playlist-endpoint-renames)
  for playlists you own or help edit.

## Install

Read the [privacy notice](PRIVACY.md) and [Spotify integration terms](TERMS.md)
before connecting your account. Phosrack's source remains GPL-3.0-or-later.

Download the file for your computer from the
[v0.3.3 release](https://github.com/minibota/phosrack/releases/tag/v0.3.3):

- **Windows 11:** choose the `.exe` installer. The `.zip` is a portable version:
  extract it into a folder and open `phosrack.exe`.
- **Linux:** choose Flatpak, or the package for your distribution: RPM for Fedora 45
  or DEB for Debian 13. These downloads are for 64-bit Intel/AMD computers.

Already using the Flatpak? Run this command in the folder containing your download:

    flatpak install --user --reinstall ./phosrack-0.3.3-x86_64.flatpak

The [getting started guide](GETTING_STARTED.md) has the installation commands,
including the extra step Fedora needs for these unsigned packages. The
[testing notes](docs/validation-0.3.3.md) explain which features have been checked
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
   screen. Review Phosrack's privacy notice and integration terms, approve access
   with the same account, then return to Phosrack.
5. In **Settings > Playback**, choose how you want to listen:
   **Play in Phosrack** or **Control another Spotify player**.

On later launches, Phosrack tries to restore your saved session. If it needs your
permission again, a notice lets you reconnect without searching through Settings.
KWallet may ask you to unlock it. Without protected storage, sign-in lasts only
for the current session. **Settings > Disconnect** removes the saved sign-in.

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
is for your own development setup. It does not remove Spotify's limits or approve
public distribution of a Spotify replacement. Wider public access requires
Spotify's approval under its
[partner access rules](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

Spotify asks for permission to control playback, read your library and playlists,
and read the account details needed by its player. If you connected an older version
of Phosrack, you may be asked to connect again to approve the added permissions.
Search results arrive in pages of up to ten items of each type. Some playlists and
playback options may be unavailable because of Spotify's account or device restrictions.

Phosrack is an independent project and is **not approved by Spotify**. Using the
official SDK, open-sourcing the code or supplying a personal Client ID does not grant
that approval. Spotify's rules about replacing its core experience and application
credentials still require clarification for public distribution. The local audio
tools are separated as a precaution; this is not a declaration of full compliance.
See the [policy review and remaining questions](docs/spotify-policy.md).

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

**Spotify mode** keeps the phosphor look and fullscreen ambient animation without
listening to the computer's output or applying Phosrack's equalizer. The fullscreen
overlay can show the current and next Spotify song; the animation is not driven by
the music.

**Local audio tools** are for non-Spotify sound that you are entitled to analyze and
process. Selecting this mode suspends Spotify in Phosrack and asks you to confirm
that use. Your saved sign-in is kept for returning to Spotify mode. Stop Spotify in other applications too: Phosrack cannot identify every
sound mixed into your speakers or headphones. Local fullscreen has no Spotify overlay.

On Linux, enable the local equalizer and choose **Phosrack EQ** as the output for
your non-Spotify audio application in system sound settings. Phosrack creates a
temporary output, and does not silently reroute applications or change the default.
Disabling EQ or closing Phosrack removes that output. Reduce the preamp when boosting
bands to avoid distortion.

On Windows, the local tools use WASAPI loopback and an optional, separately installed
Equalizer APO driver. Configure your playback device in Equalizer APO and choose its
configuration folder in Phosrack Settings. Phosrack backs up the configuration and
preserves unrelated settings; the driver is not bundled.

The local visualizer follows the selected output while visible. It includes other
applications and notifications on that output, but cannot hear a remote device.
Folding the panel or minimizing its last visible window stops monitoring. Samples
stay in memory; Phosrack does not save audio files or upload the sound.

## Development and license

See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md),
[architecture.md](docs/architecture.md), [third-party notices](THIRD_PARTY.md) and
[LICENSE](LICENSE).
Phosrack artwork and effects are original; no Winamp or Windows Media Player assets
are bundled. Spotify attribution artwork remains Spotify's property.
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
