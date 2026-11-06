# Validation of 0.3.1

This release changes the default Spotify application configuration and sign-in UI.
It embeds the project's public Client ID, offers Connect Spotify directly, and keeps
manual application configuration under Advanced Spotify settings. Each user authorizes
their own account with PKCE; no Client Secret or developer-account tokens are embedded.

## Source and builds

All published packages use application source
[`8023913`](https://github.com/minibota/phosrack/commit/80239131e903c1450c2b0118f7c9d05fd12dcd1d).

| Package | Build and tests |
| --- | --- |
| Fedora 45 RPM | [Native CI](https://github.com/minibota/phosrack/actions/runs/42311827078/job/128214864180), 17/17 CTest checks passed |
| Debian 13 DEB | [Native CI](https://github.com/minibota/phosrack/actions/runs/42311827078/job/128214864238), 17/17 CTest checks passed |
| Windows x64 EXE/ZIP | [Native MSVC CI](https://github.com/minibota/phosrack/actions/runs/42311827116), 16/16 checks and clean deployment smoke tests passed |
| Flatpak | Local KDE SDK 6.10 build from a clean `git archive` of the same commit; isolated installation and runtime checks passed |

The Flatpak CI job was still provisioning Ubuntu build tools when this release was
prepared. The published Flatpak is the independently validated local bundle. Its
application commit is `13c7b0e26fc7c979e5e1250a05720f729f1c88a75eb4f3be35422c96ae5637bd`.
The final files and SHA256 checksums are in [releases/v0.3.1](../releases/v0.3.1/).

## Application checks

The local Fedora 45 release build passed all 17 CTest checks, including Spotify
protocol and application-profile regressions, the 23-result QML suite, and Settings
and fullscreen smoke tests in English, Spanish, French and English fallback.
The connection screen was also captured and visually inspected. All 164 current
translatable strings are covered in the Spanish and French catalogs.

Configure checks accepted an empty default ID and rejected non-hexadecimal or
incorrect-length values. An independent empty-default build compiled the application
and protocol tests, passed all 16 protocol-test results, and loaded Settings without
QML warnings. Its screenshot confirmed Advanced Spotify settings expand automatically.

## Published package checks

- RPM: header/payload digests, metadata and executable version 0.3.1 verified; extracted
  application loaded Settings in Spanish and fullscreen in French with exit code 0.
- DEB: installed over 0.3.0 in a minimal Debian 13 runtime, without development SDKs;
  version 0.3.1, Settings in French and fullscreen in Spanish passed. The installed
  worker created, controlled and stopped the real equalizer sink.
- Flatpak: installed into an isolated test profile, confirmed application/AppStream
  version 0.3.1 and the embedded default ID; Settings in French and fullscreen in
  Spanish exited cleanly. The existing user installation was not changed.
- Windows: the portable ZIP passed CRC verification and contains the x64 application,
  default ID, Qt libraries and platform plugin. The installer is a valid PE/NSIS
  executable. Native CI loaded deployed Settings/fullscreen with SDK paths removed.

The [0.3.0 record](validation.md) remains available for the earlier DSP measurements
and platform validation.

## Account and hardware checks

Real Spotify browser authorization was not repeated against a live account during
this validation. Account access depends on Spotify's allowlist,
Premium requirements, and registered redirect URI. Embedding the Client ID does not
prove a particular account is authorized or remove Spotify's access restrictions.

Windows 11 audio hardware, installed Equalizer APO processing, extended sessions,
hot-unplug behavior and accessibility readers require a normal desktop. Native CI
and headless smoke tests cannot establish those results.

This is an independent development build. Technical tests do not establish Spotify
policy approval or permission to distribute every combined feature.
