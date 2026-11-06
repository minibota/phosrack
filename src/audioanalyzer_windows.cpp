// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "audioanalyzer.h"
#include "monitorformat.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <mmreg.h>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
template<class Interface> class ComPtr {
public:
    ComPtr()=default;
    ComPtr(const ComPtr &)=delete;
    ComPtr &operator=(const ComPtr &)=delete;
    ~ComPtr(){reset();}
    Interface *operator->() const{return pointer;}
    Interface *get() const{return pointer;}
    Interface **put(){reset();return &pointer;}
    void reset(){if(pointer){pointer->Release();pointer=nullptr;}}
private:
    Interface *pointer=nullptr;
};
struct ComApartment {
    explicit ComApartment(DWORD model):result(CoInitializeEx(nullptr,model)){}
    ~ComApartment(){if(SUCCEEDED(result))CoUninitialize();}
    bool usable() const{return SUCCEEDED(result)||result==RPC_E_CHANGED_MODE;}
    HRESULT result;
};
QString endpointId(IMMDevice *device) {
    LPWSTR id=nullptr;
    if(FAILED(device->GetId(&id))||!id)return {};
    const auto result=QString::fromWCharArray(id);CoTaskMemFree(id);return result;
}
QString endpointName(IMMDevice *device,const QString &fallback) {
    ComPtr<IPropertyStore> properties;
    if(FAILED(device->OpenPropertyStore(STGM_READ,properties.put())))return fallback;
    PROPVARIANT value;PropVariantInit(&value);
    QString name;
    if(SUCCEEDED(properties->GetValue(PKEY_Device_FriendlyName,&value))&&value.vt==VT_LPWSTR&&value.pwszVal)
        name=QString::fromWCharArray(value.pwszVal);
    PropVariantClear(&value);
    return name.isEmpty()?fallback:name;
}
bool mixFormat(const WAVEFORMATEX *wave,Phosrack::MonitorFormat &format) {
    if(!wave)return false;
    WORD tag=wave->wFormatTag;
    if(tag==WAVE_FORMAT_EXTENSIBLE) {
        if(wave->cbSize<sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX))return false;
        const auto *extended=reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wave);
        const GUID pcm={1,0,0x0010,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
        const GUID floating={3,0,0x0010,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
        if(std::memcmp(&extended->SubFormat,&pcm,sizeof(GUID))==0)tag=WAVE_FORMAT_PCM;
        else if(std::memcmp(&extended->SubFormat,&floating,sizeof(GUID))==0)tag=WAVE_FORMAT_IEEE_FLOAT;
        else return false;
    }
    if(tag==WAVE_FORMAT_IEEE_FLOAT&&wave->wBitsPerSample==32)format.encoding=Phosrack::MonitorEncoding::Float32;
    else if(tag==WAVE_FORMAT_PCM&&wave->wBitsPerSample==16)format.encoding=Phosrack::MonitorEncoding::Pcm16;
    else if(tag==WAVE_FORMAT_PCM&&wave->wBitsPerSample==24)format.encoding=Phosrack::MonitorEncoding::Pcm24;
    else if(tag==WAVE_FORMAT_PCM&&wave->wBitsPerSample==32)format.encoding=Phosrack::MonitorEncoding::Pcm32;
    else return false;
    format.channels=wave->nChannels;format.frameBytes=wave->nBlockAlign;format.sampleRate=wave->nSamplesPerSec;
    return format.valid();
}
}

struct AudioAnalyzer::WindowsCapture {
    std::thread thread;
    std::atomic<bool> stop{false},running{false},starting{false};
    std::mutex mutex;
    std::array<float,Phosrack::FftSize> ring{};
    unsigned write=0,fresh=0;
    int generation=0;
};

AudioAnalyzer::AudioAnalyzer(QObject *parent)
    :QObject(parent),m_windows(std::make_unique<WindowsCapture>()),m_status(tr("Visualizer paused")) {
    for(int i=0;i<Phosrack::BandCount;++i)m_bands<<0.;
    for(int i=0;i<128;++i)m_wave<<0.;
    m_frames.setInterval(33);connect(&m_frames,&QTimer::timeout,this,&AudioAnalyzer::updateFrame);
    m_routePoll.setInterval(2000);connect(&m_routePoll,&QTimer::timeout,this,&AudioAnalyzer::refreshSources);
}
AudioAnalyzer::~AudioAnalyzer(){m_active=false;m_routePoll.stop();stopCapture();}
bool AudioAnalyzer::running() const{return m_windows->running.load();}
void AudioAnalyzer::setStatus(const QString &value){if(value!=m_status){m_status=value;emit changed();}}
void AudioAnalyzer::setActive(bool value) {
    if(value==m_active)return;
    m_active=value;emit changed();
    if(!value){m_routePoll.stop();stopCapture();setStatus(tr("Visualizer paused"));return;}
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")){setStatus(tr("Waiting for audio"));return;}
    m_routePoll.start();refreshSources();
}
void AudioAnalyzer::setSource(const QString &source) {
    if(source==m_source||source.size()>4096)return;
    m_source=source;emit sourcesChanged();resolveOutput();
}
void AudioAnalyzer::refreshSources() {
    if(qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE"))return;
    ComApartment apartment(COINIT_APARTMENTTHREADED);
    ComPtr<IMMDeviceEnumerator> enumerator;
    if(!apartment.usable()||FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(enumerator.put())))) {
        setStatus(tr("Windows audio capture is unavailable."));return;
    }
    ComPtr<IMMDeviceCollection> devices;
    if(FAILED(enumerator->EnumAudioEndpoints(eRender,DEVICE_STATE_ACTIVE,devices.put()))) {
        setStatus(tr("Windows audio capture is unavailable."));return;
    }
    QVariantList sources;
    UINT count=0;devices->GetCount(&count);
    for(UINT i=0;i<count;++i) {
        ComPtr<IMMDevice> device;
        if(FAILED(devices->Item(i,device.put())))continue;
        const auto id=endpointId(device.get());
        if(!id.isEmpty())sources<<QVariantMap{{"id",id},{"name",endpointName(device.get(),id)}};
    }
    ComPtr<IMMDevice> defaultDevice;
    QString defaultId;
    if(SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender,eMultimedia,defaultDevice.put())))
        defaultId=endpointId(defaultDevice.get());
    const bool changed=sources!=m_sources||defaultId!=m_defaultSource;
    m_sources=sources;m_defaultSource=defaultId;
    if(changed)emit sourcesChanged();
    resolveOutput();
}
void AudioAnalyzer::resolveOutput() {
    const auto requested=m_source.isEmpty()?m_defaultSource:m_source;
    QString target,name;
    for(const auto &entry:m_sources) {
        const auto output=entry.toMap();
        if(output.value("id").toString()==requested){target=requested;name=output.value("name").toString();break;}
    }
    if(target!=m_resolvedSource) {
        stopCapture();m_resolvedSource=target;m_outputName=name;emit sourcesChanged();
    } else if(name!=m_outputName){m_outputName=name;emit sourcesChanged();}
    if(!m_active)return;
    if(target.isEmpty()) {
        setStatus(m_source.isEmpty()?tr("No default audio output available"):tr("Selected audio output unavailable"));return;
    }
    startCapture();
}
void AudioAnalyzer::start(){setActive(true);}
void AudioAnalyzer::stop(){setActive(false);}
void AudioAnalyzer::startCapture() {
    if(!m_active||qEnvironmentVariableIsSet("PHOSRACK_TEST_MODE")||m_resolvedSource.isEmpty()||running()||m_windows->starting)return;
    if(m_windows->thread.joinable())m_windows->thread.join();
    m_windows->stop=false;m_windows->starting=true;
    const int generation=++m_windows->generation;
    const QString endpoint=m_resolvedSource;
    setStatus(tr("Opening audio output…"));
    m_windows->thread=std::thread([this,endpoint,generation]{
        auto finish=[this,generation](bool failed) {
            m_windows->running=false;m_windows->starting=false;
            if(failed)QMetaObject::invokeMethod(this,[this,generation]{
                if(generation!=m_windows->generation||!m_active)return;
                m_frames.stop();setStatus(tr("Windows audio capture is unavailable."));emit changed();
            },Qt::QueuedConnection);
        };
        ComApartment apartment(COINIT_MULTITHREADED);
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> device;
        ComPtr<IAudioClient> client;
        const auto id=endpoint.toStdWString();
        if(!apartment.usable()||FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(enumerator.put())))||
            FAILED(enumerator->GetDevice(id.c_str(),device.put()))||
            FAILED(device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(client.put())))) {
            finish(true);return;
        }
        WAVEFORMATEX desired{};
        desired.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;desired.nChannels=2;desired.nSamplesPerSec=48000;
        desired.wBitsPerSample=32;desired.nBlockAlign=8;desired.nAvgBytesPerSec=384000;
        Phosrack::MonitorFormat format;
        HRESULT initialized=client->Initialize(AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK|AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM|AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
            1000000,0,&desired,nullptr);
        if(FAILED(initialized)) {
            // Some drivers reject auto-converted loopback. Use the actual engine
            // format and resample only the analysis samples into our 48 kHz FFT.
            client.reset();
            if(FAILED(device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(client.put())))) {
                finish(true);return;
            }
            WAVEFORMATEX *native=nullptr;
            if(FAILED(client->GetMixFormat(&native))||!native){finish(true);return;}
            const bool supported=mixFormat(native,format);
            initialized=supported?client->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,
                1000000,0,native,nullptr):E_INVALIDARG;
            CoTaskMemFree(native);
        }
        ComPtr<IAudioCaptureClient> capture;
        if(FAILED(initialized)||FAILED(client->GetService(__uuidof(IAudioCaptureClient),
            reinterpret_cast<void**>(capture.put())))||FAILED(client->Start())) {
            finish(true);return;
        }
        m_windows->running=true;m_windows->starting=false;
        QMetaObject::invokeMethod(this,[this,generation]{
            if(generation!=m_windows->generation||!m_active)return;
            setStatus(tr("Listening to system output"));m_frames.start();emit changed();
        },Qt::QueuedConnection);
        Phosrack::MonitorResampler resampler(format.sampleRate);
        std::vector<float> mono;
        bool failed=false;
        while(!m_windows->stop) {
            UINT frames=0;
            if(FAILED(capture->GetNextPacketSize(&frames))){failed=true;break;}
            while(frames&&!m_windows->stop) {
                BYTE *data=nullptr;UINT packetFrames=0;DWORD flags=0;
                if(FAILED(capture->GetBuffer(&data,&packetFrames,&flags,nullptr,nullptr))){failed=true;break;}
                mono.clear();
                for(UINT frame=0;frame<packetFrames;++frame) {
                    const bool silent=(flags&AUDCLNT_BUFFERFLAGS_SILENT)||!data;
                    const float sample=silent?0.f:Phosrack::monitorMonoFrame(data+std::size_t(frame)*format.frameBytes,
                        format.frameBytes,format);
                    resampler.append(sample,mono);
                }
                capture->ReleaseBuffer(packetFrames);
                {
                    std::lock_guard<std::mutex> lock(m_windows->mutex);
                    for(float sample:mono) {
                        m_windows->ring[m_windows->write]=sample;
                        m_windows->write=(m_windows->write+1)%Phosrack::FftSize;
                    }
                    m_windows->fresh=std::min(unsigned(Phosrack::FftSize),m_windows->fresh+unsigned(mono.size()));
                }
                if(FAILED(capture->GetNextPacketSize(&frames))){failed=true;break;}
            }
            if(failed)break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        client->Stop();finish(failed&&!m_windows->stop);
    });
}
void AudioAnalyzer::stopCapture() {
    ++m_windows->generation;m_windows->stop=true;m_frames.stop();
    if(m_windows->thread.joinable())m_windows->thread.join();
    m_windows->running=false;m_windows->starting=false;
    {
        std::lock_guard<std::mutex> lock(m_windows->mutex);
        m_windows->ring.fill(0);m_windows->write=m_windows->fresh=0;
    }
    m_level=0;m_bands.fill(0.);m_wave.fill(0.);emit changed();emit frameReady();
}
void AudioAnalyzer::updateFrame() {
    std::array<float,Phosrack::FftSize> samples{};
    {
        std::lock_guard<std::mutex> lock(m_windows->mutex);
        if(m_windows->fresh)for(int i=0;i<Phosrack::FftSize;++i)
            samples[i]=m_windows->ring[(m_windows->write+unsigned(i))%Phosrack::FftSize];
        m_windows->fresh=0;
    }
    const auto result=Phosrack::analyze(samples);m_level=result.rms;
    for(int i=0;i<Phosrack::BandCount;++i)m_bands[i]=std::max(double(result.bands[i]),m_bands[i].toDouble()*.84);
    for(int i=0;i<128;++i)m_wave[i]=result.wave[i];
    setStatus(m_level>.0005?tr("Live local audio"):tr("No signal on this output"));emit frameReady();
}
