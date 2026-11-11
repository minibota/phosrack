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
}

void AudioAnalyzer::updatePitch(const Phosrack::PitchSamples &samples) {
    m_previousPitch = Phosrack::analyzePitch(samples, m_previousPitch);
    m_pitchClasses.clear();
    for (float note : m_previousPitch)
        m_pitchClasses.append(double(note));
}

void AudioAnalyzer::resetMeter() {
    m_meterLevels = {};
    m_meterPeaks = {};
    m_peakHoldFrames = {};
}

void AudioAnalyzer::updateMeter(const Phosrack::StereoMeterFrame &meter) {
    const auto scale = [](float amplitude) {
        return std::clamp(1.0 + 20 * std::log10(std::max(double(amplitude), 1e-6)) / 50, 0.0, 1.0);
    };
    for (std::size_t i = 0; i < m_meterLevels.size(); ++i) {
        m_meterLevels[i] = std::max(scale(meter.rms[i]), m_meterLevels[i] - 0.055);
        const double peak = scale(meter.peak[i]);
        if (peak > 0 && peak >= m_meterPeaks[i]) {
            m_meterPeaks[i] = peak;
            m_peakHoldFrames[i] = 12;
        } else if (m_peakHoldFrames[i] > 0) {
            --m_peakHoldFrames[i];
        } else {
            m_meterPeaks[i] = std::max(peak, m_meterPeaks[i] - 0.045);
        }
    }
}

void AudioAnalyzer::updateFrame() {
    std::array<float, Phosrack::FftSize> samples{};
    Phosrack::PitchSamples pitch;
    const bool pitchDue = m_pitchEnabled && m_pitchCountdown == 0;
    Phosrack::StereoMeterFrame meter;
    const bool fresh = readAudioFrame(samples, pitchDue ? &pitch : nullptr, meter);
    updateMeter(meter);
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
