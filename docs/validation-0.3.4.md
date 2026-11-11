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
