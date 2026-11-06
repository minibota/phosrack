# Desktop packaging

Phosrack 0.3.1 installs the application with embedded QML/translations, a small native
PipeWire EQ worker, a desktop entry,
AppStream metadata, an original SVG icon, documentation and the GPL license. Packages
do not bundle the host's Qt/KDE libraries. Its desktop ID is `io.phosrack.Phosrack`.

## Spotify application configuration

All targets use the same CMake default public Client ID, including native Linux,
Windows and Flatpak. Release builds therefore offer **Connect Spotify** directly.
The ID identifies an application; it is not an account password, access token or
Client Secret. Each user completes their own PKCE browser authorization.

For a fork or another developer application, override the default:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DPHOSRACK_SPOTIFY_CLIENT_ID=YOUR_32_CHARACTER_CLIENT_ID
```

An explicit empty value, `-DPHOSRACK_SPOTIFY_CLIENT_ID=`, builds without a bundled
application ID and leaves manual configuration in Settings > Advanced Spotify settings. A cached
CMake value persists when reconfiguring an existing build directory; set the value
explicitly or use a fresh build directory when changing the intended application.

For Flatpak forks, add the same `-DPHOSRACK_SPOTIFY_CLIENT_ID=...` entry to the
application module's `config-opts`. The shipped manifest inherits CMake's default.
Register `http://127.0.0.1:43821/callback` for the selected application. Never compile
or distribute a Client Secret. Bundling a public ID does not expand Spotify's
[development-mode access](https://developer.spotify.com/documentation/web-api/concepts/quota-modes).

## RPM

Build on Fedora 45 with the dependencies from README, then:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
(cd build && cpack -G RPM)
sudo rpm --define '_pkgverify_level digest' -Uvh ./build/phosrack-0.3.1-1.fc45.x86_64.rpm
```

CPack uses RPM's ELF dependency scanner and adds Qt Quick/PipeWire tools as runtime
requirements. Native packages require a compatible distribution/ABI.

Local development RPMs are unsigned. Fedora 45's strict RPM policy rejects unsigned
packages even when their SHA256 digests are valid. The per-command override above
keeps digest and dependency verification without changing global policy. Install missing
runtime dependencies first using the distribution's signed packages:

```sh
sudo dnf install qt6-qtdeclarative qt6-qtsvg qt6-qtwayland kf6-kwallet pipewire pipewire-libs pipewire-utils
```

A public release should be signed with its publisher's trusted key; no signing identity
or key has been invented for these local development artifacts.

## Debian

Build inside Debian 13 (trixie), using its own Qt 6.8 / KF6 libraries:

```sh
podman build -t phosrack-debian -f packaging/debian/Dockerfile .
container_id=$(podman create phosrack-debian)
podman cp "$container_id:/build/phosrack_0.3.1_amd64.deb" .
podman rm "$container_id"
sudo apt install ./phosrack_0.3.1_amd64.deb
```

The Dockerfile also works with Docker. CPack uses `dpkg-shlibdeps` to compute ELF
library dependencies, with explicit QML/PipeWire package dependencies. A DEB made by
renaming or converting the Fedora RPM is not supported.

## Flatpak

The manifest uses `org.kde.Platform` / `org.kde.Sdk` 6.10 and a local source directory:

```sh
flatpak install --user flathub org.kde.Platform//6.10 org.kde.Sdk//6.10
flatpak-builder --user --force-clean --repo=repo flatpak-build packaging/flatpak/io.phosrack.Phosrack.json
flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo repo phosrack-0.3.1-x86_64.flatpak io.phosrack.Phosrack
flatpak install --user --reinstall ./phosrack-0.3.1-x86_64.flatpak
flatpak run io.phosrack.Phosrack
```

Required permissions are declared explicitly:

- Network for Spotify HTTPS and the 127.0.0.1 OAuth callback.
- Wayland / fallback X11, IPC and DRI for Qt rendering.
- `xdg-run/pipewire-0` for direct output-monitor samples/routing.
- `org.kde.kwalletd6` on the session bus for KWallet.

The manifest does not grant home access, the whole session bus, microphones as selectable
inputs, or `flatpak-spawn --host`. Access to the PipeWire socket gives the process audio
server access; Phosrack itself restricts enumeration/capture to output sinks.

The KDE runtime provides PipeWire client tools, modules and the client library.
The EQ worker links that public library and owns its two audio streams and filters;
it does not require a filter-chain daemon, a manager permission or a host command
wrapper. The helper processes audio locally and connects to the host audio server.
Actual build results are recorded in [the validation notes](validation-0.3.1.md). Test playback,
wallet authorization and live visuals on a normal desktop after installation.

## Publication

Source and versioned development installers are published at
https://github.com/minibota/phosrack. Before submitting to Flathub or a distribution,
verify application-ID ownership, complete publisher/contact metadata and the
respective review requirements, and sign artifacts with a trusted publisher key.
The local-source manifest does not submit anything to Flathub. A public release or
bundled Spotify ID does not establish Spotify approval or wider API access.

References: [CPack](https://cmake.org/cmake/help/latest/module/CPack.html),
[Flatpak permissions](https://docs.flatpak.org/en/latest/sandbox-permissions.html),
[KDE runtime manifest example](https://github.com/flathub/org.kde.dolphin/blob/master/org.kde.dolphin.json).
