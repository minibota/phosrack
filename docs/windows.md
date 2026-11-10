# Windows 11 builds

Phosrack has a native Windows build using Qt 6 and MSVC 2022. The Windows workflow
creates a portable ZIP and an NSIS installer, uploaded as the
`phosrack-0.3.3-windows-x64` Actions artifact and attached to the versioned release.

## Features and runtime status

- Native Qt/QML playback, search, queue, library, equalizer controls and fullscreen UI.
- Integrated Spotify Web Playback SDK in Qt WebEngine, with external-player fallback.
- Browser OAuth and Windows Credential Manager session storage.
- Spotify mode uses ambient visuals; it does not capture sound or apply Phosrack EQ.
- Separate Local audio tools use WASAPI output loopback and optional Equalizer APO
  processing for non-Spotify sound. No microphone is selected. The APO driver must
  be installed and configured separately for the playback device.

A native build or clean deployment smoke test does not verify DRM playback or audio
hardware. **Real Windows integrated audio playback, WASAPI and installed Equalizer APO
processing have not yet been validated for v0.3.3.** See the
[validation notes](validation-0.3.3.md) for the exact checks completed. The audible
Linux feasibility experiment does not establish Windows playback support on a given PC.

## Verified media capabilities

The previous v0.3.2 Windows CI runtime probe for application source
`4f048bcf4218d863f002bfd45d6eeb7c30b495f4` used Qt 6.8.3. It reported a secure
loopback context, Opus/Vorbis decoding and successful Widevine MediaKeys creation
for those formats. **AAC decoding is unavailable in this Qt build**, and AAC
MediaKeys initialization returned `NotSupportedError`.

These checks do not establish authenticated Spotify audio playback. Treat integrated
Windows playback as experimental. If the player cannot initialize, select
**Settings > Playback > Control another Spotify player** and keep that player open.
Choosing a Widevine DLL does not add an audio decoder missing from the Qt build.

## Spotify and DRM setup

v0.3.3 does not bundle a shared Spotify Client ID. Configure your own application in
**Settings > Spotify application setup**, enable **Web API** and **Web Playback SDK**
in its developer dashboard, and register exactly `http://127.0.0.1:43821/callback`.
Click **Connect Spotify**, accept the integration terms and authorize your Premium account. A Client Secret is
never needed. Follow the full [setup instructions](../GETTING_STARTED.md#connect-spotify),
including any reauthorization requested for older grants. On subsequent launches,
Phosrack restores a saved session and displays a prompt if browser authorization is
needed. Spotify-imposed API waits remain in force after restarting.

**Play in Phosrack** is the default playback mode. It needs a compatible Widevine
module and media codecs; no official Spotify player needs to remain open. Phosrack
tries common installed Chrome locations for `widevinecdm.dll`. If needed, use
**Settings > Playback > Choose Widevine module…** to select a module from an existing
trusted installation, then restart. Phosrack neither downloads nor bundles that module.
A detected DLL does not guarantee compatibility with the packaged Qt WebEngine runtime.

If DRM or playback initialization fails, **Control another Spotify player** uses an
active official desktop app, web player or other Spotify Connect device. That player
must remain running. Both modes retain Spotify's Premium and application-access limits.

## Build and package on Windows

Install Visual Studio 2022 with Desktop development with C++, Qt 6.8.3 MSVC x64
including **Qt WebEngine**, **Qt WebChannel** and their Qt Positioning dependency,
CMake, Ninja and NSIS. Use a native x64 MSVC developer PowerShell with Qt's bin
directory on PATH:

    cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DPHOSRACK_SPOTIFY_CLIENT_ID=
    cmake --build build-win --parallel 2
    ctest --test-dir build-win --output-on-failure
    ./packaging/windows/package.ps1

The script deploys Qt DLLs, QML imports, plugins and the compiler runtime. It also
checks WebEngineProcess, Chromium resource packs and locale data, then runs the
deployed tree with SDK PATH/search overrides removed. CPack creates ZIP and NSIS
packages under `dist`, and the final ZIP layout is checked. The Windows workflow
also passes `-VerifyInstaller`: it installs the NSIS package into a unique temporary
folder, launches the installed app, validates the Start-menu shortcut target and
uninstalls it. Normal local packaging does not run this installation check unless
the switch is supplied. No KWallet or Linux PipeWire dependency is used by Windows.
The workflow reports WebEngine codec configuration; that is diagnostic information,
not a substitute for an authenticated playback test.

The portable ZIP can be extracted into a user-writable folder. The installer uses
the normal Windows application installation flow. Development packages are unsigned;
no publisher code-signing identity is configured.

For a private build or fork, replace the empty CMake option with
`-DPHOSRACK_SPOTIFY_CLIENT_ID=YOUR_32_CHARACTER_CLIENT_ID` if you want to bundle your
own public ID. The deployment/package reconfigure preserves that cached value.
An older build cache can retain the previous ID, so set the option explicitly when
reconfiguring. See [application configuration](packaging.md#spotify-application-configuration)
for the remaining Spotify access restrictions. Official v0.3.3 builds use an empty default.

## Equalizer APO

Use **Settings > Local audio tools** for non-Spotify sound. This suspends the
Spotify session in Phosrack, retaining saved credentials for when you return. Stop
Spotify in other players too; device-level processing cannot distinguish every app.

Install Equalizer APO from its official project and configure its audio processing
object for your playback device. In Phosrack Settings, set the APO configuration
directory. The integration must be active before slider changes affect output.
Do not overwrite an unrelated APO configuration manually.

Phosrack writes its own bounded gain configuration, preserves unrelated settings
and creates a backup. The driver remains a separate installation. Its effect applies
to the configured Windows output. Returning to Spotify mode disables Phosrack's
EQ configuration first; if that fails, connection is blocked until the EQ is bypassed.
Unrelated APO filters are not controlled by Phosrack. See the validation record
before assuming hardware compatibility has been tested.

## References

- [Qt Windows requirements](https://doc.qt.io/qt-6.8/windows.html)
- [Qt Windows deployment](https://doc.qt.io/qt-6.8/windows-deployment.html)
- [Qt WebEngine DRM](https://doc.qt.io/qt-6.8/qtwebengine-features.html#html5-drm)
- [CPack NSIS generator](https://cmake.org/cmake/help/latest/cpack_gen/nsis.html)
