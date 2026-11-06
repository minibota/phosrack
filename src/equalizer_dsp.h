// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace Phosrack {
class EqualizerDsp {
public:
    static constexpr std::array<double,10> frequencies{31.25,62.5,125,250,500,1000,2000,4000,8000,16000};
    void configure(const std::array<double,10> &gains,double preamp) {
        amplitude=std::pow(10.,std::clamp(preamp,-12.,12.)/20.);
        constexpr double pi=3.14159265358979323846;
        for(int i=0;i<10;++i){
            if(gains[i]==lastGains[i]&&initialized)continue;
            const double a=std::pow(10.,std::clamp(gains[i],-12.,12.)/40.);
            const double omega=2*pi*frequencies[i]/48000.;
            const double alpha=std::sin(omega)/(2*1.4142135623730951);
            const double a0=1+alpha/a;
            auto &band=bands[i];
            band.b0=(1+alpha*a)/a0;
            band.b1=-2*std::cos(omega)/a0;
            band.b2=(1-alpha*a)/a0;
            band.a1=band.b1;
            band.a2=(1-alpha/a)/a0;
            lastGains[i]=gains[i];
        }
        initialized=true;
    }
    float sample(float input,int channel) {
        double x=std::isfinite(input)?double(input)*amplitude:0.;
        for(auto &band:bands){
            const double y=band.b0*x+band.z1[channel];
            band.z1[channel]=band.b1*x-band.a1*y+band.z2[channel];
            band.z2[channel]=band.b2*x-band.a2*y;
            x=y;
        }
        return std::isfinite(x)?float(x):0.f;
    }
private:
    struct Band {
        double b0=1,b1=0,b2=0,a1=0,a2=0;
        std::array<double,2> z1{},z2{};
    };
    std::array<Band,10> bands;
    std::array<double,10> lastGains{};
    double amplitude=1;
    bool initialized=false;
};
}
