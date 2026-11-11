// SPDX-License-Identifier: GPL-3.0-or-later
#include "audioanalyzer.h"
#include <algorithm>

void AudioAnalyzer::setPitchEnabled(bool value) {
    if (value == m_pitchEnabled) return;
    m_pitchEnabled = value;
    resetPitch();
    emit changed();
    emit frameReady();
}

void AudioAnalyzer::resetPitch() {
    m_pitchClasses.clear();
    m_previousPitch.fill(0);
    m_pitchCountdown = m_idleFrames = 0;
    m_chordCandidate = m_chordRoot = -1;
    m_minorChord = false;
}

void AudioAnalyzer::updatePitch(const Phosrack::PitchSamples &samples) {
    const auto pitch = Phosrack::analyzePitch(samples, m_previousPitch);
    m_previousPitch = pitch.notes;
    m_pitchClasses.clear();
    for (float note : pitch.notes)
        m_pitchClasses.append(double(note));

    const int candidate = pitch.chordRoot < 0 ? -1 : pitch.chordRoot * 2 + int(pitch.minor);
    // Two consecutive estimates avoid flashing a chord during short transients.
    if (candidate < 0 || candidate == m_chordCandidate) {
        m_chordRoot = pitch.chordRoot;
        m_minorChord = pitch.minor;
    }
    m_chordCandidate = candidate;
}

void AudioAnalyzer::updateFrame() {
    std::array<float, Phosrack::FftSize> samples{};
    Phosrack::PitchSamples pitch;
    const bool pitchDue = m_pitchEnabled && m_pitchCountdown == 0;
    const bool fresh = readAudioFrame(samples, pitchDue ? &pitch : nullptr);
    const auto frame = Phosrack::analyze(samples);
    m_level = frame.rms;
    for (int i = 0; i < Phosrack::BandCount; ++i)
        m_bands[i] = std::max(double(frame.bands[i]), m_bands[i].toDouble() * 0.84);
    for (int i = 0; i < 128; ++i)
        m_wave[i] = frame.wave[i];

    if (m_pitchEnabled) {
        if (!fresh && ++m_idleFrames >= 6) resetPitch();
        if (fresh) {
            m_idleFrames = 0;
            if (pitchDue) {
                updatePitch(pitch);
                m_pitchCountdown = 3;
            }
        }
        if (m_pitchCountdown > 0) --m_pitchCountdown;
    }

    setStatus(m_level > 0.0005 ? tr("Live local audio") : tr("No signal on this output"));
    emit frameReady();
}
