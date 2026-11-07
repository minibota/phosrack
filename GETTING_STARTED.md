# Getting started with Phosrack

Phosrack is a QML + C++ desktop music rack with integrated Spotify playback, search,
queue, library, a local equalizer and fullscreen visualization. The interface follows
the system language (English, Spanish or French); unsupported languages use English.

## Install a development package

Download an installer from the
[v0.3.2 release](https://github.com/minibota/phosrack/releases/tag/v0.3.2). The
[versioned package index](releases/v0.3.2/) contains download links and checksums.
Choose the package for your operating system and CPU architecture. See the
[0.3.2 validation notes](docs/validation-0.3.2.md) for completed checks and remaining
platform limitations.

- Fedora 45 x86_64: install the RPM after its runtime dependencies.
- Debian 13 amd64: install the DEB with your package manager.
- Linux Flatpak: install the local Flatpak bundle.
- Windows 11 x64: use the Windows installer or extract the portable ZIP and open
  `phosrack.exe`. Windows packages are produced by the native MSVC workflow.

Local development RPMs are unsigned. On Fedora 45, install the signed runtime
dependencies, then use a per-command digest check without changing global policy:

    sudo dnf install qt6-qtdeclarative qt6-qtsvg qt6-qtwayland qt6-qtwebengine qt6-qtwebchannel kf6-kwallet pipewire pipewire-libs pipewire-utils
    sudo rpm --define '_pkgverify_level digest' -Uvh ./phosrack-0.3.2-1.fc45.x86_64.rpm

On Debian 13:

    sudo apt install ./phosrack_0.3.2_amd64.deb

For Flatpak (including updates to an existing user installation):

    flatpak install --user --reinstall ./phosrack-0.3.2-x86_64.flatpak

See [docs/packaging.md](docs/packaging.md) for build instructions and dependencies.
Installing Phosrack does not install Widevine or create a Spotify developer application.

## Connect Spotify

You need **Spotify Premium** and your own Spotify developer application's public
**Client ID**. This is a one-time setup; Phosrack remembers your ID. v0.3.2 has no
shared default ID.

1. Open the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
   using your Premium account. Create or select an application for your personal
   Phosrack installation.
2. Enable **Web API** and **Web Playback SDK** and add exactly
   `http://127.0.0.1:43821/callback` to its redirect URIs. Save the settings.
3. Copy its **Client ID**, not its Client Secret. In Phosrack Settings, open
   **Spotify application setup** and paste it into **Client ID (no Client Secret)**.
4. Click **Connect Spotify**, then authorize the same account in your browser.
5. Keep **Settings > Playback > Play in Phosrack** selected. Check the player status
   and DRM setup below, then play an item from **Search** or **Library**.

The public Client ID identifies your application; it is not a password. Never enter
a Client Secret or access token. Session credentials use protected operating-system
storage when available. Disconnect removes the saved token.

**Upgrading from v0.3.1:** if you used its shared ID, configure your own application.
An explicitly saved personal ID is preserved. Enable **Web Playback SDK** for that
application and reconnect when prompted to grant the additional streaming/profile
permissions. The callback URL is unchanged.

Spotify requires the development application's owner and the listening account to
have Premium for this playback setup. New development applications allow up to five
authenticated users. For a different listening account, add it under **Users Management**
in the dashboard first. Browser sign-in can succeed while API access remains blocked.
Using your own application avoids relying on a shared Phosrack tester list; it does
not remove Spotify's per-application limits. See
[Spotify's access rules](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

## Playback and Widevine

**Play in Phosrack** is the default. The official Web Playback SDK runs inside Qt
WebEngine and plays through this computer's audio output. The official Spotify app
and website do not need to remain open. If there is no current playback session,
choose a track in Search or Library; Play can also transfer an existing session to
Phosrack when you request it.

This mode needs a compatible **Widevine DRM module**, supported media codecs and an
internet connection. Phosrack does not include or download Widevine. It looks for an
existing compatible Chrome installation. In **Settings > Playback**, the detected
path and player status help identify missing setup. Use **Choose Widevine module…**
to select an existing trusted `libwidevinecdm.so` on Linux or `widevinecdm.dll` on
Windows, then restart Phosrack. **Auto-detect** restores automatic module selection;
restart after changing that selection too. Finding a file does not guarantee that
its version is compatible with the installed Qt WebEngine runtime.

The Flatpak can read Chrome's standard `/opt/google/chrome/WidevineCdm` directory;
other host locations may need selection through the file chooser and must remain
accessible inside the sandbox. See [Flatpak packaging](docs/packaging.md#flatpak).

If the integrated player cannot initialize, use **Retry player** after correcting
its configuration, or select **Control another Spotify player**. In external mode,
start playback in the official Spotify desktop app, the
[web player](https://open.spotify.com/) or another Spotify Connect device, and keep
that player running. Phosrack controls it; audio stays on the selected device.
Switching modes does not bypass Premium or application access requirements.

## Equalizer

Choose a preset or edit the ten bands and preamp. Boosting bands can clip loud audio;
reduce the preamp level to leave headroom.

On Linux, enable EQ and select **Phosrack EQ** as the output for the application
producing the sound in the system sound settings: Phosrack in integrated mode, or
Spotify/your browser in external mode. The effect works locally without a Spotify
connection. Phosrack creates a temporary processing output and does not silently
reroute apps or change the system default. Disabling EQ or closing Phosrack removes
that output.

Windows uses the optional Equalizer APO integration described in the application
and [Windows guide](docs/windows.md). Equalizer APO must be installed and configured
for your playback device separately; it is not bundled with Phosrack.

## Visualizer and fullscreen

The visualizer follows the default local output while visible. Output override and
reduced-motion settings are available in Settings. It includes audio from other
applications on that output, and does not receive sound from remote Spotify devices.

Click the fullscreen icon to open the large visualizer. Its overlay shows the
current track and the next item in the Spotify queue. Escape or the close button
returns to the rack. Unavailable song information is shown explicitly when Spotify
is disconnected or the queue is empty. Minimizing the rack stops its visual analysis;
integrated music playback continues.

The compact rack has its own draggable title strip, Settings, minimize and close
controls. Fullscreen is available from the visualizer, rather than maximizing the rack.

## Project status

Source is licensed GPL-3.0-or-later. Technical implementation does not imply Spotify
approval: distribution of the combined application remains subject to Spotify's
terms and any required permission. See the README and validation notes before
publishing a release.
