# Windows 11 builds

Phosrack supports a Windows-specific source build with Qt 6 and MSVC 2022.
The native Windows workflow creates a portable ZIP and an NSIS installer, then
uploads them as the phosrack-0.3.1-windows-x64 Actions artifact.

## Features

- Native Qt/QML playback, search, queue, library, equalizer controls and fullscreen UI.
- Browser OAuth with Windows Credential Manager session storage.
- WASAPI loopback for visualization of the default playback output, without a microphone.
- Optional Equalizer APO integration for device-level EQ. Equalizer APO must be installed
  and configured separately for the playback device; its driver is not bundled.

An installer produced in CI is not evidence that real audio hardware was tested.
See [the validation notes](validation-0.3.1.md) for the checks actually completed for each delivery.

## Build and package on Windows

Install Visual Studio 2022 with Desktop development with C++, Qt 6.8.3 MSVC x64,
CMake, Ninja and NSIS. Use a native x64 MSVC developer PowerShell with Qt's bin
directory on PATH:

    cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-win --parallel 2
    ctest --test-dir build-win --output-on-failure
    ./packaging/windows/package.ps1

The script deploys Qt DLLs, QML imports, plugins and the compiler runtime, then runs
the deployed tree with SDK PATH/search overrides removed. CPack creates the ZIP and
installer under dist. No KWallet or Linux PipeWire dependency is used by Windows.

The portable ZIP can be extracted into a user-writable folder. The installer uses
the normal Windows application installation flow. Development packages are unsigned;
a public release should use its publisher's trusted code-signing identity.

The standard build includes Phosrack's public Spotify application ID. Users click
**Connect Spotify** and authorize their own account; developer overrides are under
Settings > Advanced Spotify settings. For a fork, add
`-DPHOSRACK_SPOTIFY_CLIENT_ID=YOUR_32_CHARACTER_CLIENT_ID` to the first CMake command.
The deployment/package reconfigure preserves that cached value. See
[application configuration](packaging.md#spotify-application-configuration) for
Spotify access restrictions and builds without a bundled ID.

## Equalizer APO

Enable the Equalizer APO integration only after installing it from its official
project and configuring its audio processing object for your playback device.
Use Phosrack Settings to select the APO configuration directory. Follow the
application's setup status; the integration must be active before slider changes
can affect output. Do not overwrite an unrelated APO configuration manually.

The driver remains a separate installation. Phosrack writes its own bounded gain
configuration and preserves unrelated settings. Check [the validation notes](validation-0.3.1.md) for the exact
level of Windows runtime and hardware testing completed.

## References

- https://doc.qt.io/qt-6.8/windows.html
- https://doc.qt.io/qt-6.8/windows-deployment.html
- https://cmake.org/cmake/help/latest/cpack_gen/nsis.html
