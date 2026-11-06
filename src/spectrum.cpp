// SPDX-License-Identifier: GPL-3.0-or-later
#include "spectrum.h"
#include <algorithm>
#include <cmath>
#include <complex>
namespace Phosrack {
SpectrumFrame analyze(const std::array<float,FftSize> &samples, int sampleRate) {
    constexpr double pi=3.14159265358979323846;
    std::array<std::complex<double>,FftSize> fft{};
    SpectrumFrame out;
    double power=0;
    for(int i=0;i<FftSize;++i) {
        const double x=std::isfinite(samples[i])?std::clamp(double(samples[i]),-1.0,1.0):0;
        power+=x*x;
        fft[i]=x*(.5-.5*std::cos(2*pi*i/(FftSize-1)));
        if(i%16==0) out.wave[i/16]=float(x);
    }
    out.rms=float(std::sqrt(power/FftSize));
    for(int i=1,j=0;i<FftSize;++i) {
        int bit=FftSize>>1;
        for(;j&bit;bit>>=1) j^=bit;
        j^=bit;
        if(i<j) std::swap(fft[i],fft[j]);
    }
    for(int len=2;len<=FftSize;len<<=1) {
        const std::complex<double> root=std::polar(1.0,-2*pi/len);
        for(int i=0;i<FftSize;i+=len) {
            std::complex<double> w=1;
            for(int j=0;j<len/2;++j) {
                const auto a=fft[i+j], b=fft[i+j+len/2]*w;
                fft[i+j]=a+b;fft[i+j+len/2]=a-b;w*=root;
            }
        }
    }
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
}
