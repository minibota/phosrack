// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Phosrack {
enum class MonitorEncoding { Float32, Pcm16, Pcm24, Pcm32 };
struct MonitorFormat {
    MonitorEncoding encoding = MonitorEncoding::Float32;
    unsigned channels = 2;
    unsigned frameBytes = 8;
    unsigned sampleRate = 48000;
    unsigned sampleBytes() const {
        switch(encoding) {
        case MonitorEncoding::Float32: case MonitorEncoding::Pcm32:return 4;
        case MonitorEncoding::Pcm16:return 2;
        case MonitorEncoding::Pcm24:return 3;
        }
        return 0;
    }
    bool valid() const {
        return channels>=1 && channels<=32 && frameBytes>=sampleBytes()*channels &&
            frameBytes<=512 && sampleRate>=8000 && sampleRate<=384000;
    }
};

inline float monitorMonoFrame(const unsigned char *frame,std::size_t bytes,const MonitorFormat &format) {
    if(!frame||!format.valid()||bytes<format.frameBytes)return 0;
    double sum=0;
    for(unsigned channel=0;channel<format.channels;++channel) {
        const auto *sample=frame+channel*format.sampleBytes();
        double value=0;
        switch(format.encoding) {
        case MonitorEncoding::Float32: {
            float decoded=0;std::memcpy(&decoded,sample,sizeof(decoded));value=decoded;break;
        }
        case MonitorEncoding::Pcm16: {
            std::int16_t decoded=0;std::memcpy(&decoded,sample,sizeof(decoded));value=double(decoded)/32768.;break;
        }
        case MonitorEncoding::Pcm24: {
            std::int32_t decoded=std::int32_t(sample[0])|(std::int32_t(sample[1])<<8)|(std::int32_t(sample[2])<<16);
            if(decoded&0x800000)decoded-=0x1000000;
            value=double(decoded)/8388608.;break;
        }
        case MonitorEncoding::Pcm32: {
            std::int32_t decoded=0;std::memcpy(&decoded,sample,sizeof(decoded));value=double(decoded)/2147483648.;break;
        }
        }
        if(std::isfinite(value))sum+=std::clamp(value,-1.,1.);
    }
    return float(sum/format.channels);
}

// Meter-only streaming conversion into the analyzer's fixed 48 kHz timebase.
// This never changes the audio sent to a playback device.
class MonitorResampler {
public:
    explicit MonitorResampler(unsigned sourceRate=48000)
        :m_step(sourceRate>=8000&&sourceRate<=384000?double(sourceRate)/48000.:1.){}
    void append(float sample,std::vector<float> &output) {
        if(!std::isfinite(sample))sample=0;
        sample=std::clamp(sample,-1.f,1.f);
        if(!m_started) {
            m_started=true;m_previous=sample;output.push_back(sample);m_position=m_step;return;
        }
        while(m_position<=1.) {
            output.push_back(float(m_previous+(sample-m_previous)*m_position));
            m_position+=m_step;
        }
        m_position-=1.;m_previous=sample;
    }
private:
    double m_step=1,m_position=0;
    float m_previous=0;
    bool m_started=false;
};
}
