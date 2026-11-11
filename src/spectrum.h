// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <cmath>
namespace Phosrack {
constexpr int FftSize=2048;
constexpr int BandCount=48;
constexpr int PitchFftSize=8192;
constexpr int PitchClassCount=12;
using PitchSamples = std::array<float, PitchFftSize>;
using PitchClasses = std::array<float, PitchClassCount>;
struct SpectrumFrame { std::array<float,BandCount> bands{}; std::array<float,128> wave{}; float rms=0; };
SpectrumFrame analyze(const std::array<float,FftSize> &samples, int sampleRate=48000);

PitchClasses analyzePitch(const PitchSamples &samples, const PitchClasses &previous = {});

struct StereoMeterFrame {
    std::array<float, 2> rms{};
    std::array<float, 2> peak{};
};

class AudioHistory {
public:
    void append(float sample, float left, float right) {
        m_samples[m_write] = sample;
        m_write = (m_write + 1) % m_samples.size();
        m_fresh = true;
        ++m_meterSamples;
        const std::array<float, 2> channels{left, right};
        for (std::size_t i = 0; i < channels.size(); ++i) {
            const float magnitude = std::abs(channels[i]);
            m_power[i] += double(magnitude) * magnitude;
            if (m_peak[i] < magnitude) m_peak[i] = magnitude;
        }
    }
    StereoMeterFrame takeMeterFrame() {
        StereoMeterFrame frame;
        if (m_meterSamples) {
            for (std::size_t i = 0; i < m_power.size(); ++i)
                frame.rms[i] = float(std::sqrt(m_power[i] / m_meterSamples));
            frame.peak = m_peak;
        }
        m_power = {};
        m_peak = {};
        m_meterSamples = 0;
        return frame;
    }
    bool takeFresh() {
        const bool fresh = m_fresh;
        m_fresh = false;
        return fresh;
    }
    template<std::size_t Size>
    void copyRecent(std::array<float, Size> &samples) const {
        static_assert(Size <= PitchFftSize);
        const auto start = (m_write + m_samples.size() - Size) % m_samples.size();
        for (std::size_t i = 0; i < Size; ++i)
            samples[i] = m_samples[(start + i) % m_samples.size()];
    }
private:
    PitchSamples m_samples{};
    std::size_t m_write = 0;
    bool m_fresh = false;
    std::size_t m_meterSamples = 0;
    std::array<double, 2> m_power{};
    std::array<float, 2> m_peak{};
};
}
