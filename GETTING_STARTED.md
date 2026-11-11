# Getting started with Phosrack

Phosrack is a QML + C++ desktop music rack with integrated Spotify playback, search,
queue, a ten-band equalizer and fullscreen audio-reactive visuals. The interface follows
the system language (English, Spanish or French); unsupported languages use English.

## Install a development package

Read the [privacy notice](PRIVACY.md) and [Spotify integration terms](TERMS.md)
before connecting Spotify.

Download an installer from the
[v0.3.5 release](https://github.com/minibota/phosrack/releases/tag/v0.3.5). The
[versioned package index](releases/v0.3.5/) contains download links and checksums.
Choose the package for your operating system and CPU architecture. See the
[0.3.5 validation notes](docs/validation-0.3.5.md) for completed checks and remaining
platform limitations.

- Fedora 45 x86_64: install the RPM after its runtime dependencies.
- Debian 13 amd64: install the DEB with your package manager.
- Linux Flatpak: install the local Flatpak bundle.
- Windows 11 x64: use the Windows installer or extract the portable ZIP and open
  `phosrack.exe`. Windows packages are produced by the native MSVC workflow.

Local development RPMs are unsigned. On Fedora 45, install the signed runtime
dependencies, then use a per-command digest check without changing global policy:

    sudo dnf install qt6-qtdeclarative qt6-qtsvg qt6-qtwayland qt6-qtwebengine qt6-qtwebchannel kf6-kwallet pipewire pipewire-libs pipewire-utils
    sudo rpm --define '_pkgverify_level digest' -Uvh ./phosrack-0.3.5-1.fc45.x86_64.rpm

On Debian 13:

    sudo apt install ./phosrack_0.3.5_amd64.deb

For Flatpak (including updates to an existing user installation):

    flatpak install --user --reinstall ./phosrack-0.3.5-x86_64.flatpak

See [docs/packaging.md](docs/packaging.md) for build instructions and dependencies.
Installing Phosrack does not install Widevine or create a Spotify developer application.

## Connect Spotify

You need **Spotify Premium** and your own Spotify developer application's public
**Client ID**. This is a one-time setup; Phosrack remembers your ID. v0.3.5 has no
shared default ID.

1. Open the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
   using your Premium account. Create or select an application for your personal
   Phosrack installation.
2. Enable **Web API** and **Web Playback SDK** and add exactly
   `http://127.0.0.1:43821/callback` to its redirect URIs. Save the settings.
3. Copy its **Client ID**, not its Client Secret. In Phosrack Settings, open
   **Spotify application setup** and paste it into **Client ID (no Client Secret)**.
4. Click **Connect Spotify**, review and accept the integration terms, then authorize
   the same account in your browser. The confirmation page tells you to return to Phosrack.
5. Keep **Settings > Playback > Play in Phosrack** selected. Check the player status
   and DRM setup below, then play an item from **Search** or **Library**.

The public Client ID identifies your application; it is not a password. Never enter
a Client Secret or access token. Session credentials use protected operating-system
storage when available. On later launches, Phosrack restores your session and
shows a notice if another browser authorization is needed. KWallet may need to be
unlocked. Without protected storage, the session is not saved between launches.
**Settings > Disconnect** removes the saved token and clears account data in the app.

**Upgrading from v0.3.1:** if you used its shared ID, configure your own application.
An explicitly saved personal ID is preserved. Enable **Web Playback SDK** for that
application and reconnect when prompted to grant the additional streaming/profile
permissions. The callback URL is unchanged.

Spotify requires the development application's owner and the listening account to
have Premium for this playback setup. New development applications allow up to five
authenticated users. For a different listening account, add it under **Users Management**
in the dashboard first. Browser sign-in can succeed while API access remains blocked.
Your Client ID is for personal development, not a way to bypass access rules.
Development quotas are shared by all applications owned by the same developer
account; generating another ID does not reset them. See
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

## If Spotify asks you to wait

A Spotify limit message means the service has temporarily paused API access. It
does not mean your password or Premium subscription is wrong. Phosrack shows the
remaining wait and keeps it across restarts. Let it finish; reconnecting, restarting
or creating another Client ID does not remove Spotify's restriction. The app will
resume eligible requests after the wait without a rapid retry loop.

Spotify has both short-term request limits and development quotas. Other apps under
the same developer account can consume the latter too. Phosrack reduces background
requests, but cannot guarantee that Spotify will never apply a limit. See the
[technical policy review](docs/spotify-policy.md).

## Liked songs and playback recovery

Use the heart beside the playback controls to add or remove the displayed song
from Liked Songs. If you connected an earlier version, disconnect and reconnect
Spotify once to grant library write access.

Phosrack shows the active song or the last song it saw, including its position.
Press Play to resume. Queue shows the latest entries Spotify supplies. If Spotify
closed the previous playback session, its exact manual queue may be unavailable.

## Local audio tools and equalizer

The equalizer and visualizer can run alongside Spotify playback. The visualizer
reacts to the full mix on the selected local output.

Choose a preset or edit the ten bands. The preamp starts at 0 dB each time you
turn on the equalizer, and changing presets keeps the current preamp level.
Reduce the preamp when boosting bands to leave headroom.

On Linux, enable EQ and select **Phosrack EQ** as the output for your
audio app in the system sound settings. Phosrack creates a temporary processing
output and does not reroute apps or change the system default. Disabling EQ or
closing Phosrack removes that output.

Windows uses the optional Equalizer APO integration described in the application
and [Windows guide](docs/windows.md). Install and configure Equalizer APO for your
playback device separately. Phosrack disables its filters when you turn off the EQ.

## Visualizer and fullscreen

The fullscreen view can show the current Spotify song and the next track when
one is queued. The visualizer reacts to the selected computer output. Choose the
stereo meter or the Notes display in the visualizer panel. The stereo meter
shows each channel independently and holds the peak briefly.
The visualizer cannot hear sound from remote playback devices.

Output selection and reduced motion are in Settings. Hiding or folding the local
visualizer, or minimizing its last visible window, stops analysis. Integrated
Spotify playback can continue with the main rack minimized.

Use the fullscreen icon to open the large view; Escape or the close button returns
to the rack. The compact rack has its own draggable title strip, Settings, minimize
and close controls.

## Project status

Source is licensed GPL-3.0-or-later. Technical implementation does not imply Spotify
approval: distribution of the combined application remains subject to Spotify's
terms and any required permission. A personal Client ID does not authorize wider
public access. See the [policy review](docs/spotify-policy.md), README and validation
notes before publishing a release.
