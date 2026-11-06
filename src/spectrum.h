// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <vector>
namespace Phosrack {
constexpr int FftSize=2048;
constexpr int BandCount=48;
struct SpectrumFrame { std::array<float,BandCount> bands{}; std::array<float,128> wave{}; float rms=0; };
SpectrumFrame analyze(const std::array<float,FftSize> &samples, int sampleRate=48000);
}
