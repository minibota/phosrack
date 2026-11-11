# Phosrack 0.3.4 validation

## Linux live audio

Both new effects were exercised with two excerpts of recorded music routed
through a temporary PipeWire output. The test used Phosrack's real capture and
analysis code and rendered both QML effects at the rack's new height of 142 px.
The grid followed changes in the 48 frequency bands. The notes display tracked
changes in pitch-class strengths and cleared the chord after the audio stopped.

After smoothing pitch across consecutive windows, a probable chord appeared on
about 30% of active frames in the first excerpt, up from 12%. It appeared on
about 3% of active frames in the more ambiguous second excerpt. These percentages
measure how often a label appears. Chord accuracy on the recordings was not
scored against a transcription. Automated fixtures verify known major and minor
triads, a harmonically rich single note, an ambiguous mixture and silence.

The Linux live test used local recordings. It did not exercise authenticated
Spotify playback, Windows audio capture or Equalizer APO hardware processing.

## Build and package checks

All installers were built from
[`f7a262ec012bcbb7a1a57d0627630fe3d878c1f6`](https://github.com/minibota/phosrack/tree/f7a262ec012bcbb7a1a57d0627630fe3d878c1f6).

- [Fedora and Debian CI](https://github.com/minibota/phosrack/actions/runs/43060633654)
  compiled, ran CTest and produced the RPM and DEB. RPM digests and DEB package
  metadata were checked after download.
- [Windows CI](https://github.com/minibota/phosrack/actions/runs/43060633746)
  passed 18/18 tests, verified the portable ZIP, and checked the NSIS installation,
  launch and Start menu shortcut. The Qt 6.8.3 probe could initialize Widevine
  for Opus and Vorbis. AAC decoding and AAC Widevine initialization were unavailable.
- The locally built Flatpak installed in an isolated KDE 6.10 test environment
  and passed version, French Settings and Spanish fullscreen smoke checks.
  Its Flatpak commit is `867e77e758d8c4fd463b9411c43feb4c7c3240606210185b417729bf276a3340`.
- [SHA-256 checksums](../releases/v0.3.4/SHA256SUMS.txt) cover all five packages.
