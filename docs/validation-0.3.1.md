# Validation of 0.3.1

This release changes the default Spotify application configuration and sign-in UI.
It embeds the project's public Client ID, offers Connect Spotify directly, and keeps
manual application configuration under Advanced Spotify settings. Each user authorizes
their own account with PKCE; no Client Secret or developer-account tokens are embedded.

## Current status

The local Fedora 45 release build passed all 17 CTest checks, including Spotify
protocol and application-profile regressions, the 23-result QML suite, and Settings
and fullscreen smoke tests in English, Spanish, French and English fallback.
The connection screen was also captured and visually inspected. All 164 current
translatable strings are covered in the Spanish and French catalogs.

Configure checks accepted an empty default ID and rejected non-hexadecimal or
incorrect-length values. CI builds and package/deployed runtime checks for 0.3.1
are pending; their source revision, workflow records and exact artifact checks
will be added after completion. The [0.3.0 record](validation.md) describes the
preceding release and is not evidence of 0.3.1 package validation.

## Account and hardware checks

Real Spotify browser authorization and account access depend on Spotify's allowlist,
Premium requirements, and registered redirect URI. Embedding the Client ID does not
prove a particular account is authorized or remove Spotify's access restrictions.

Windows 11 audio hardware, installed Equalizer APO processing, extended sessions,
hot-unplug behavior and accessibility readers require a normal desktop. Native CI
and headless smoke tests cannot establish those results.

This is an independent development build. Technical tests do not establish Spotify
policy approval or permission to distribute every combined feature.
