// SPDX-License-Identifier: GPL-3.0-or-later
#include "spectrum.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>
namespace Phosrack {
namespace {
constexpr double pi=3.14159265358979323846;
double clean(float sample) {
    return std::isfinite(sample) ? std::clamp(double(sample), -1.0, 1.0) : 0;
}

template<std::size_t Size>
auto transform(const std::array<float, Size> &samples) {
    static const auto window = [] {
        std::array<double, Size> values{};
        for (std::size_t i = 0; i < Size; ++i)
            values[i] = 0.5 - 0.5 * std::cos(2 * pi * i / (Size - 1));
        return values;
    }();
    std::array<std::complex<double>, Size> fft;
    for (std::size_t i = 0; i < Size; ++i)
        fft[i] = clean(samples[i]) * window[i];
    for(std::size_t i=1,j=0;i<Size;++i) {
        auto bit=Size>>1;
        for(;j&bit;bit>>=1) j^=bit;
        j^=bit;
        if(i<j) std::swap(fft[i],fft[j]);
    }
    for(std::size_t len=2;len<=Size;len<<=1) {
        const std::complex<double> root=std::polar(1.0,-2*pi/len);
        for(std::size_t i=0;i<Size;i+=len) {
            std::complex<double> w=1;
            for(std::size_t j=0;j<len/2;++j) {
                const auto a=fft[i+j], b=fft[i+j+len/2]*w;
                fft[i+j]=a+b;fft[i+j+len/2]=a-b;w*=root;
            }
        }
    }
    return fft;
}
}

SpectrumFrame analyze(const std::array<float,FftSize> &samples, int sampleRate) {
    const auto fft = transform(samples);
    SpectrumFrame out;
    double power=0;
    for(int i=0;i<FftSize;++i) {
        const double x=clean(samples[i]);
        power+=x*x;
        if(i%16==0) out.wave[i/16]=float(x);
    }
    out.rms=float(std::sqrt(power/FftSize));
    for(int band=0;band<BandCount;++band) {
        const double lo=30*std::pow(16000.0/30, double(band)/BandCount);
        const double hi=30*std::pow(16000.0/30, double(band+1)/BandCount);
        const int first=std::clamp(int(lo*FftSize/sampleRate),1,FftSize/2-1);
        const int last=std::clamp(int(std::ceil(hi*FftSize/sampleRate)),first+1,FftSize/2);
        double peak=0;
        for(int k=first;k<last;++k) peak=std::max(peak,std::abs(fft[k])*4/FftSize);
        out.bands[band]=float(std::clamp((20*std::log10(std::max(peak,1e-9))+70)/70,0.0,1.0));
    }
    return out;
}

PitchClasses analyzePitch(const PitchSamples &samples, const PitchClasses &previous) {
    constexpr double binHz = 48000.0 / PitchFftSize;
    constexpr int firstBin = int(75 / binHz);
    constexpr int lastBin = int(1800 / binHz);
    const auto fft = transform(samples);
    std::array<double, lastBin + 2> magnitudes{};
    for (int i = firstBin - 1; i <= lastBin + 1; ++i)
        magnitudes[i] = std::abs(fft[i]);
    const double peak = *std::max_element(magnitudes.begin(), magnitudes.end());
    PitchClasses result{};
    if (peak < 0.8) return result;

    struct Peak { double frequency; double magnitude; };
    std::vector<Peak> lowerPeaks;
    std::array<double, PitchClassCount> strengths{};
    for (int i = firstBin; i <= lastBin; ++i) {
        const double center = magnitudes[i];
        if (center < peak * 0.10 || center <= magnitudes[i - 1] || center < magnitudes[i + 1])
            continue;
        // Interpolate the spectral peak between FFT bins before rounding to a note.
        const double left = std::log(std::max(1e-12, magnitudes[i - 1]));
        const double middle = std::log(center);
        const double right = std::log(std::max(1e-12, magnitudes[i + 1]));
        const double shift = std::clamp(0.5 * (left - right) / (left - 2 * middle + right), -0.5, 0.5);
        const double frequency = (i + shift) * binHz;
        const double note = 69 + 12 * std::log2(frequency / 440.0);
        const int nearest = int(std::round(note));
        if (std::abs(note - nearest) > 0.40) continue;
        const bool overtone = std::any_of(lowerPeaks.begin(), lowerPeaks.end(), [=](const Peak &lower) {
            const int harmonic = int(std::round(frequency / lower.frequency));
            return harmonic >= 2 && harmonic <= 6 && lower.magnitude >= center * 0.55
                && std::abs(frequency - harmonic * lower.frequency) <= std::max(binHz, frequency * 0.006);
        });
        if (!overtone) lowerPeaks.push_back({frequency, center});
        strengths[nearest % PitchClassCount] += center * (overtone ? 0.12 : 1.0) / std::sqrt(frequency / 220.0);
    }

    const double framePeak = *std::max_element(strengths.begin(), strengths.end());
    if (framePeak == 0) return result;
    const float blend = *std::max_element(previous.begin(), previous.end()) > 0 ? 0.50f : 0.f;
    for (int note = 0; note < PitchClassCount; ++note) {
        const float current = float(strengths[note] / framePeak);
        result[note] = (1 - blend) * current + blend * previous[note];
    }

    return result;
}
}
