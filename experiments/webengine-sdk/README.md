# Isolated Qt WebEngine / Spotify playback experiment

This experiment tests whether Qt WebEngine can host Spotify's official Web
Playback SDK. It is separate from Phosrack's C++/QML application, build,
configuration, installed packages and saved Spotify sessions. Running it does not
change Phosrack's playback backend.

The small Python harness uses PySide6 bindings to the native Qt WebEngine library.
It is a diagnostic harness, not a proposal to rewrite Phosrack in Python. A working
result would still need validation in QML/C++, the target Qt version, Windows, and
each Linux package format before integrating this backend.

## Requirements

- Python 3 and PySide6 with Qt WebEngine and Qt WebChannel.
- A working Qt graphical environment; offscreen diagnostics are optional.
- An existing compatible Widevine installation and the required audio codecs.
  Qt does not bundle Widevine. This experiment does not download, copy or
  redistribute it, spoof the browser identity, or disable DRM/TLS/sandbox checks.
- Spotify Premium for real playback.
- Your own Spotify developer application with **Web Playback SDK** and **Web API**
  enabled, and this exact registered redirect URI:

      http://127.0.0.1:43821/callback

Your account must have access to that developer application. Development-mode
user restrictions still apply. A public Client ID is needed; a Client Secret is
never used.

## Diagnostics without Spotify login

From this directory:

    python3 probe.py --timeout 25 --report /tmp/phosrack-webengine-diagnostics.json

The temporary window checks a secure loopback origin, audio codec support,
Widevine access and media-key creation, and loading the Spotify SDK. Passing these
checks does **not** prove that Spotify authenticates or plays audio successfully.

The listener uses `127.0.0.1:43821`; another active Phosrack authorization attempt
or probe must not occupy that port. The page is served at an unpredictable path.

On a machine supporting Qt offscreen rendering, diagnostic-only execution can use:

    QT_QPA_PLATFORM=offscreen python3 probe.py --headless --timeout 25

The headless option only hides the window. It does not remove the runtime's
rendering, DRM or sandbox requirements.

## Test actual playback

Start a song in Spotify first so that an active playback context exists. Then run:

    python3 probe.py --client-id YOUR_32_CHARACTER_CLIENT_ID --login --exercise --timeout 600 --report /tmp/phosrack-webengine-playback.json

Complete Spotify authorization in the external browser. The probe requests
`streaming`, profile access required by the SDK, and playback read/control scopes.
It opens no embedded Spotify login page and reads no existing Phosrack token store.

With `--exercise`, it will:

1. Register a local device named **Phosrack WebEngine Probe**.
2. Record the previous Spotify device and whether it was playing, in memory.
3. Transfer the current playback context to the probe at 25% player volume.
4. Sample playback position, pause, resume and seek five seconds forward (or
   backward when near the end of a track).
5. Attempt to restore playback to the original device and its original play/pause
   state. It does not rewind the music to its starting position.

Without `--exercise`, use **Play here**, **Pause**, **Resume** and **Restore previous
player** manually. SDK errors are recorded by category. If there is no active
playback context, start a song in Spotify and click **Play here** again.

Closing the window or reaching the timeout attempts to restore the original
device before exiting. Restoration is best effort: the original device may have
gone away, the network may fail, or forcefully terminating the process may prevent
cleanup. Use Spotify's device picker to restore playback manually if needed.

## Evidence and limits

The optional JSON report contains only structural diagnostics: Qt version, codec
and DRM results, SDK status, HTTP status codes, and playback position/state.
It contains no OAuth codes, access/refresh tokens, Client IDs, device IDs, track
names or callback URLs. Browser console output is suppressed to keep those values
out of ordinary logs. OAuth state and tokens remain in memory; the WebEngine
profile is off the record. The report is overwritten during the run.

A useful pass requires **all** of the following, not merely `sdk_ready`:

- Successful authorization and SDK registration.
- A successful transfer to the probe.
- Playback state showing an advancing position while unpaused.
- Audible output from Qt WebEngine and successful transport controls.

A Linux host pass does not establish Windows compatibility, redistributable DRM
availability, Flatpak compatibility, public Spotify API access, or permission for
Phosrack's complete feature set. The current released application still uses an
external Spotify player.

See [the recorded Linux results](RESULTS.md) for the authenticated playback test
and the remaining validation work.

## Primary documentation

- [Spotify Web Playback SDK](https://developer.spotify.com/documentation/web-playback-sdk)
- [SDK reference](https://developer.spotify.com/documentation/web-playback-sdk/reference)
- [Spotify authorization with PKCE](https://developer.spotify.com/documentation/web-api/tutorials/code-pkce-flow)
- [Qt WebEngine DRM](https://doc.qt.io/qt-6.8/qtwebengine-features.html#html5-drm)
- [Spotify development and extended quota modes](https://developer.spotify.com/documentation/web-api/concepts/quota-modes)
