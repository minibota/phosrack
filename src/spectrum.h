// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
namespace Phosrack {
constexpr int FftSize=2048;
constexpr int BandCount=48;
constexpr int PitchFftSize=8192;
constexpr int PitchClassCount=12;
using PitchSamples = std::array<float, PitchFftSize>;
using PitchClasses = std::array<float, PitchClassCount>;
struct SpectrumFrame { std::array<float,BandCount> bands{}; std::array<float,128> wave{}; float rms=0; };
SpectrumFrame analyze(const std::array<float,FftSize> &samples, int sampleRate=48000);

struct PitchFrame {
    PitchClasses notes{};
    int chordRoot = -1;
    bool minor = false;
};
PitchFrame analyzePitch(const PitchSamples &samples, const PitchClasses &previous = {});

class AudioHistory {
public:
    void append(float sample) {
        m_samples[m_write] = sample;
        m_write = (m_write + 1) % m_samples.size();
        m_fresh = true;
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
};
}
