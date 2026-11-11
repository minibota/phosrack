# Privacy notice

Effective 10 November 2026 · Phosrack

Phosrack is a desktop application maintained through the
[minibota/phosrack project](https://github.com/minibota/phosrack). It has no account
server, advertising service or analytics service of its own. Spotify and your
browser or DRM provider operate their services under their own policies.

## When you connect Spotify

Phosrack contacts Spotify directly to sign in, search, display your playback, queue,
saved music and playlists, and carry out the controls you choose. Spotify receives
these requests and ordinary connection information, such as your IP address.
The integrated Spotify player also requires account/profile permissions, including
email permission; Phosrack does not use your email to send messages or create a
mailing list. Album images are fetched from Spotify's image servers.

Your Spotify password is entered on Spotify's page in your regular browser, not in
Phosrack. A temporary authorization code returns to a loopback address on your own
computer. Phosrack uses PKCE and checks a random state value before accepting it.
The confirmation page has no tracking, external fonts or analytics.

Access tokens and current account content remain in application memory. A refresh
token, which lets Phosrack restore your session, is saved in KWallet on Linux or
Windows Credential Manager when available. If protected storage is unavailable,
Phosrack does not save that token to a plain-text file. The integrated player uses
an in-memory web profile with no persistent cookies or disk cache; this profile is
destroyed when the player is stopped or disconnected.

## Settings saved on your computer

Phosrack saves your public Client ID, interface and audio preferences, selected
Widevine location, permission-acceptance version, and session-related settings.
It also saves your explicit sign-out choice and Spotify's API wait deadline so restarting cannot discard an active
pause. These are local settings, not an uploaded profile. They do not contain your
Spotify password, Client Secret or access token. Installer and runtime dependencies
may have their own settings or logs.

## Local audio tools

While the local visualizer is visible, it receives sound from the selected computer
output, including during Spotify playback. That can include other apps and notifications; it cannot identify every
source in the mix. Audio samples are processed in memory and are not saved or
uploaded. Folding or hiding the visualizer, or minimizing its last visible window,
stops monitoring. EQ applies only when you enable the local tool. Windows Equalizer
APO configuration is backed up; unrelated configuration is preserved.

## Disconnect, deletion and browser cookies

Choose **Settings > Disconnect** to stop Phosrack's Spotify session, remove its saved
refresh token and clear the account content held by the application. Disconnecting
also prevents silent restoration of that saved session, even if the protected store
is locked or refuses deletion. If deletion fails, Phosrack shows instructions to
remove its entry manually in KWallet or Windows Credential Manager; it will not
reuse that entry. Interface preferences,
public application configuration and any active API wait deadline remain. Remove
Phosrack's settings through your operating system if you also want to erase those
preferences.

You can additionally remove the application's access from your
[Spotify account's Apps page](https://www.spotify.com/account/apps/).
Authorization happens in your regular browser, whose Spotify cookies and history
are managed by that browser; Phosrack cannot clear them. You can clear cookies or
change their settings there. See [Spotify's privacy policy](https://www.spotify.com/legal/privacy-policy/)
for Spotify's own processing and controls.

## Questions

Use the [project's issue tracker](https://github.com/minibota/phosrack/issues) to
ask a general privacy question or request a private contact channel. Do not post
passwords, tokens, account details or other private information in a public issue.
Security reports follow [SECURITY.md](SECURITY.md). Spotify is a third-party
beneficiary of this notice as it relates to the Spotify integration and may enforce
those provisions directly.
