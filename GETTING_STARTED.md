# Getting started with Phosrack

Phosrack is a QML + C++ desktop music rack with playback controls, search, queue,
library, a local equalizer and fullscreen visualization. The interface follows the
system language (English, Spanish or French); unsupported languages use English.

## Install a development package

Download an installer from the
[v0.3.1 release](https://github.com/minibota/phosrack/releases/tag/v0.3.1), or the
[versioned package folder](releases/v0.3.1/). Choose the package for your operating
system and CPU architecture. See
[0.3.1 validation notes](docs/validation-0.3.1.md) for the exact validation status.

- Fedora 45 x86_64: install the RPM after its runtime dependencies.
- Debian 13 amd64: install the DEB with your package manager.
- Linux Flatpak: install the local Flatpak bundle.
- Windows 11 x64: use the Windows installer or extract the portable ZIP and open
  phosrack.exe. Windows packages are produced by the native MSVC workflow.

Local development RPMs are unsigned. On Fedora 45, install the signed runtime
dependencies, then use a per-command digest check without changing global policy:

    sudo dnf install qt6-qtdeclarative qt6-qtsvg qt6-qtwayland kf6-kwallet pipewire pipewire-libs pipewire-utils
    sudo rpm --define '_pkgverify_level digest' -Uvh ./phosrack-0.3.1-1.fc45.x86_64.rpm

On Debian 13:

    sudo apt install ./phosrack_0.3.1_amd64.deb

For Flatpak (including updates to an existing user installation):

    flatpak install --user --reinstall ./phosrack-0.3.1-x86_64.flatpak

See [docs/packaging.md](docs/packaging.md) for build instructions and dependencies.

## Connect Spotify

You need **Spotify Premium** and your own Spotify developer application's public
**Client ID**. This is a one-time setup; Phosrack remembers your ID. Current source
builds have no shared default ID.

1. Open the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
   using your Premium account. Create or select an application for your personal
   Phosrack installation.
2. Enable **Web API** and add exactly `http://127.0.0.1:43821/callback` to its
   redirect URIs. Save the application settings.
3. Copy its **Client ID**, not its Client Secret. In Phosrack Settings, open
   **Spotify application setup** and paste it into **Client ID (no Client Secret)**.
4. Click **Connect Spotify**, then authorize the same account in your browser.
5. Start playback in the official Spotify desktop app or
   [web player](https://open.spotify.com/), and leave it running in the background.
   Phosrack controls that player; it does not stream Spotify audio itself.

**Using the downloadable v0.3.1 package?** It still has the earlier shared ID and
calls the setup section **Advanced Spotify settings**. Expand that section, replace
the displayed ID with your own, then connect again. The current source change does
not update already published installers or an installed copy automatically.

The public Client ID identifies your application; it is not a password. Never enter
a Client Secret or access token. Session credentials use protected operating-system
storage when available. Disconnect removes the saved token.

Your development application's owner must retain Premium, and playback controls
require Premium for the listening account. Spotify permits up to five authenticated
users per new development application. If signing in with a different account,
add it under **Users Management** in your application's dashboard. Browser sign-in
can succeed while API access remains blocked for an unlisted account.

Using your own application avoids relying on Phosrack's shared tester list, but
does not remove Spotify's limits or approve the application for public access.
See [Spotify's access rules](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

To supply an optional default when compiling your own build, see
[Spotify application configuration](docs/packaging.md#spotify-application-configuration).

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
