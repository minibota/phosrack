# Desktop packaging

Phosrack 0.3.4 installs the application with embedded QML/translations and playback
bridge, a Linux PipeWire EQ worker, desktop entry, AppStream metadata, icon,
documentation and GPL license. Its desktop ID is `io.phosrack.Phosrack`.

RPM/DEB use their host distribution's Qt/KDE libraries, including Qt WebEngine and
WebChannel. Flatpak uses the KDE runtime and WebEngine BaseApp. Windows deploys the
Qt runtime, WebEngine helper/resources and compiler runtime. **Widevine is not included
in any package.** Runtime DRM, codec and authenticated playback checks are distinct
from build and packaging checks; see [validation-0.3.4.md](validation-0.3.4.md).

## Spotify application configuration

Official v0.3.4 builds use **no bundled Spotify Client ID** on Linux, Windows and
Flatpak. Each user enters their own public ID under **Settings > Spotify application
setup**, then completes PKCE browser authorization. The ID identifies an application;
it is not an account password, access token or Client Secret. The setting persists.

To ensure a build has no bundled ID, including when reusing a build directory:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DPHOSRACK_SPOTIFY_CLIENT_ID=
```

CMake caches this value. A build directory from v0.3.1 may still contain the earlier
shared ID; changing the source default does not clear the cache. Set the value
explicitly or use a fresh build directory.

For a private build or fork using your own application, an optional default remains:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DPHOSRACK_SPOTIFY_CLIENT_ID=YOUR_32_CHARACTER_CLIENT_ID
```

Users of such a build can override its ID under **Settings > Advanced Spotify settings**.
For a Flatpak fork, set the same option in the application module's `config-opts`.
Bundling an ID does not grant public access: the application's allowlist and Premium
requirements still apply. Developer quotas are shared across an owner's apps, so
changing IDs is not a quota reset. A personal/default ID does not establish approval
for broader distribution; see [policy review](spotify-policy.md). Never compile or
distribute a Client Secret.

Enable **Web API** and **Web Playback SDK** for the chosen application and register
exactly `http://127.0.0.1:43821/callback`. Upgrading users must reconnect for the new
streaming/profile permissions. Historical v0.3.1 packages are unchanged; v0.3.4 users
who relied on their shared ID must configure their own application. See
[Spotify setup](../GETTING_STARTED.md#connect-spotify) and
[Spotify's access rules](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

## RPM

Build on Fedora 45 with the dependencies from README, then:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPHOSRACK_SPOTIFY_CLIENT_ID=
cmake --build build --parallel
ctest --test-dir build --output-on-failure
(cd build && cpack -G RPM)
sudo rpm --define '_pkgverify_level digest' -Uvh ./build/phosrack-0.3.4-1.fc45.x86_64.rpm
```

CPack uses RPM's ELF dependency scanner with explicit Qt Quick, WebEngine, WebChannel
and PipeWire runtime requirements. Native packages require a compatible distribution/ABI.

Local development RPMs are unsigned. Fedora 45's strict RPM policy rejects unsigned
packages even when their SHA256 digests are valid. The per-command override above
retains digest and dependency verification without changing global policy. Install
missing dependencies using the distribution's signed packages first:

```sh
sudo dnf install qt6-qtdeclarative qt6-qtsvg qt6-qtwayland qt6-qtwebengine qt6-qtwebchannel kf6-kwallet pipewire pipewire-libs pipewire-utils
```

No signing identity or key has been invented for these development artifacts.

## Debian

Build inside Debian 13 (trixie), using its own Qt 6.8 / KF6 libraries:

```sh
podman build -t phosrack-debian -f packaging/debian/Dockerfile .
container_id=$(podman create phosrack-debian)
podman cp "$container_id:/build/phosrack_0.3.4_amd64.deb" .
podman rm "$container_id"
sudo apt install ./phosrack_0.3.4_amd64.deb
```

The Dockerfile also works with Docker and includes Qt WebEngine/WebChannel development
packages. CPack uses `dpkg-shlibdeps` for ELF dependencies and explicit QML, WebEngine
helper and PipeWire runtime dependencies. Renaming/converting the Fedora RPM is not
supported.

## Flatpak

The manifest uses `org.kde.Platform` / `org.kde.Sdk` 6.10,
`io.qt.qtwebengine.BaseApp` 6.10 and a local source directory:

```sh
flatpak install --user flathub org.kde.Platform//6.10 org.kde.Sdk//6.10 io.qt.qtwebengine.BaseApp//6.10
flatpak-builder --user --force-clean --repo=repo flatpak-build packaging/flatpak/io.phosrack.Phosrack.json
flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo repo phosrack-0.3.4-x86_64.flatpak io.phosrack.Phosrack
flatpak install --user --reinstall ./phosrack-0.3.4-x86_64.flatpak
flatpak run io.phosrack.Phosrack
```

The BaseApp supplies WebEngine libraries, helper process and data absent from the KDE
runtime alone. Its cleanup script removes build-only components; the manifest sets
`QTWEBENGINEPROCESS_PATH=/app/bin/QtWebEngineProcess` for the deployed helper.

Required permissions are explicit:

- Network for Spotify HTTPS, the fixed OAuth callback and private SDK loopback page.
- Wayland / fallback X11, IPC and DRI for Qt rendering.
- PulseAudio socket for WebEngine playback, including PipeWire's PulseAudio service.
- `xdg-run/pipewire-0` for direct output-monitor samples and the local EQ worker.
- `org.kde.kwalletd6` on the session bus for KWallet.
- Read-only `/opt/google/chrome/WidevineCdm` access to detect a user's existing Chrome
  DRM module. The module itself is not copied into the bundle.

The manifest does not grant the whole home directory, whole session bus or
`flatpak-spawn --host`. Audio-server access is broader than a capture-only permission;
Phosrack's local-audio visualizer enumerates and captures output sinks only after
entering Local audio tools. Spotify mode disables monitoring and Phosrack EQ. The
application does not offer microphone inputs.

A host Widevine module in another location must be selected through the file chooser
and remain accessible to the sandbox on subsequent launches. A path visible on the host
is not automatically readable inside Flatpak. Restart after changing the selected
module. The BaseApp's media/DRM preflight alone does not prove account authorization
or audible playback; the validation record identifies these checks separately.

The KDE runtime supplies PipeWire client tools, modules and client library. The EQ
worker owns its streams and filters without a filter-chain daemon, manager permission
or host command wrapper. Test Spotify playback and wallet authorization separately
from non-Spotify local routing and reactive visuals on a normal desktop after installation.

## Windows

The native MSVC workflow deploys WebEngineProcess, Chromium resource packs, locales,
Qt DLLs and QML imports before producing an NSIS installer and portable ZIP. It checks
the deployed tree with SDK search paths removed. Widevine remains an existing runtime
installation chosen by the user. See [Windows builds](windows.md) for instructions and
hardware/DRM validation limits.

## Publication

Source and installers are published at [minibota/phosrack](https://github.com/minibota/phosrack).
The v0.3.4 release targets RPM, DEB, Flatpak, Windows EXE and portable ZIP packages.
Attach installers to the GitHub release. Keep a versioned `releases/v0.3.4/` index and
SHA256 checksums in Git; packages over GitHub's 100 MiB file limit must not be committed
to the repository. Old release artifacts and validation records remain unchanged.

Before Flathub or distribution submission, verify application-ID ownership, complete
publisher/contact metadata and the respective review requirements. The local-source
manifest does not submit anything to Flathub. Public source/downloads do not establish
Spotify approval or wider API access. Include [PRIVACY.md](../PRIVACY.md) and
[TERMS.md](../TERMS.md) in release documentation and retain the initial connection
consent flow. The [policy review](spotify-policy.md) records unresolved public
distribution questions. WebEngine/Chromium licenses and notices must
accompany redistributed runtime components; DRM is not redistributed.

References: [CPack](https://cmake.org/cmake/help/latest/module/CPack.html),
[Flatpak permissions](https://docs.flatpak.org/en/latest/sandbox-permissions.html),
[Qt WebEngine deployment](https://doc.qt.io/qt-6.8/qtwebengine-deploying.html).
