# Third-party materials

## Spotify attribution artwork

`package/contents/ui/assets/spotify-logo-white.svg` is the unmodified
`Full_Logo_White_RGB.svg` from Spotify's official
[2024 full-logo archive](https://developer.spotify.com/images/guidelines/design/2024-spotify-full-logo.zip),
downloaded 10 November 2026. The original file is preserved byte for byte; Phosrack
renders it proportionally as an attribution link.

Spotify's logo and trademarks belong to Spotify. They are not licensed under
Phosrack's GPL license. Use is subject to Spotify's
[Design and Branding Guidelines](https://developer.spotify.com/documentation/design)
and [Developer Terms](https://developer.spotify.com/terms). Their appearance identifies
Spotify content and does not imply endorsement of Phosrack.

The official white monochrome asset is used on the application's dark backgrounds,
with at least 70 logical pixels of logo width and surrounding clear space. Artwork
is not recolored to match the phosphor theme.

## Runtime components

Native packages use their platform's Qt, KDE and PipeWire libraries. Windows and
Flatpak packages include runtime components with their corresponding licenses and
notices. Each release's `THIRD_PARTY.md` records the packaged components and source
information. Widevine is not included. Spotify's Web Playback SDK is fetched from
Spotify at runtime rather than copied into the application.
