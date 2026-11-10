# Security and privacy

For user-facing data practices and deletion instructions, see [PRIVACY.md](PRIVACY.md).
Spotify connection requires acceptance of the [integration terms](TERMS.md).

## Credentials and session data

Refresh tokens use KWallet on Linux (Phosrack folder) or Windows Credential Manager,
namespaced by the public Client ID. If protected storage is unavailable, tokens
remain in memory. Access tokens, PKCE verifiers and OAuth state are never written
to settings or application logs. Release builds have no shared default Client ID;
a public Client ID identifies the user's development application and is not a
password. Never paste a Client Secret into Phosrack.

OAuth uses the system browser and a loopback callback, PKCE and validated state.
The confirmation page contains no credentials or third-party resources. The
integrated SDK runs in an off-the-record WebEngine profile with memory-only cache
and cookies, destroyed when the player stops. Disconnect clears account content
and removes the saved refresh token. Local preferences and Spotify's API wait
deadline remain so reconnection cannot discard a server-requested pause.

## Local audio

Spotify mode disables Phosrack's system-audio analysis and EQ. The separate Local
audio tools mode suspends Spotify and requires acknowledgement that only
non-Spotify sound will be processed. This is not source detection: other apps can
still send Spotify sound to a system output, and users must stop them themselves.

The local monitor runs only while its visualizer is visible, using the chosen
output. Samples are processed in memory, never saved or uploaded. Audio processes
use fixed executables and argument lists without a shell; capture targets are
selected from discovered output nodes. The local EQ creates a temporary Linux
sink or maintains an isolated Equalizer APO include with a backup on Windows.

## Reporting a vulnerability

Use the [repository Security tab](https://github.com/minibota/phosrack/security)
for available private reporting options. If none are listed, open an issue asking
for a private channel without describing the vulnerability or exposing account
data. Do not post live tokens. Security fixes target the latest development release.
