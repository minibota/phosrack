// SPDX-License-Identifier: GPL-3.0-or-later
// Private local DSP helper. It processes only audio explicitly routed into its
// virtual output. Controls stay in this process, never requiring manager rights.
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSocketNotifier>
#include <QTimer>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include "equalizer_dsp.h"
#include <array>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace {
volatile std::sig_atomic_t interrupted=0;
void interruptedSignal(int) { interrupted=1; }
constexpr size_t RingSize=32768;
static_assert(std::atomic<double>::is_always_lock_free && std::atomic<uint64_t>::is_always_lock_free,
    "The equalizer requires lock-free audio control and buffer counters");
struct Worker {
    QCoreApplication *app=nullptr;
    pw_thread_loop *loop=nullptr;
    pw_context *context=nullptr;
    pw_core *core=nullptr;
    pw_registry *registry=nullptr;
    pw_stream *capture=nullptr,*playback=nullptr;
    spa_hook registryListener{},coreListener{},captureListener{},playbackListener{};
    std::array<std::atomic<double>,10> gains{};
    std::atomic<double> preamp{0};
    Phosrack::EqualizerDsp dsp;
    std::array<float,RingSize> ring{};
    std::atomic<uint64_t> read{0},write{0};
    int failure=0;
    bool ready=false;
};
void fail(Worker &w){
    w.failure=3;QMetaObject::invokeMethod(w.app,[&w]{w.app->exit(w.failure);},Qt::QueuedConnection);
}
void onCapture(void *data) {
    auto &w=*static_cast<Worker*>(data);
    auto *buffer=pw_stream_dequeue_buffer(w.capture);if(!buffer)return;
    if(buffer->buffer->n_datas<1){pw_stream_queue_buffer(w.capture,buffer);return;}
    const auto &source=buffer->buffer->datas[0];
    if(source.data&&source.chunk){
        const size_t offset=std::min(source.chunk->offset,source.maxsize);
        const size_t bytes=std::min(size_t(source.chunk->size),size_t(source.maxsize)-offset);
        const auto *input=reinterpret_cast<const float*>(static_cast<const uint8_t*>(source.data)+offset);
        const size_t samples=bytes/8*2;
        std::array<double,10> gains;
        for(int i=0;i<10;++i)gains[i]=w.gains[i].load(std::memory_order_relaxed);
        w.dsp.configure(gains,w.preamp.load(std::memory_order_relaxed));
        auto write=w.write.load(std::memory_order_relaxed);
        const auto read=w.read.load(std::memory_order_acquire);
        for(size_t i=0;i<samples;i+=2){
            const auto left=w.dsp.sample(input[i],0),right=w.dsp.sample(input[i+1],1);
            if(write-read+2<=RingSize){w.ring[write%RingSize]=left;w.ring[(write+1)%RingSize]=right;write+=2;}
        }
        w.write.store(write,std::memory_order_release);
    }
    pw_stream_queue_buffer(w.capture,buffer);
}
void onPlayback(void *data) {
    auto &w=*static_cast<Worker*>(data);
    auto *buffer=pw_stream_dequeue_buffer(w.playback);if(!buffer)return;
    if(buffer->buffer->n_datas<1){pw_stream_queue_buffer(w.playback,buffer);return;}
    auto &output=buffer->buffer->datas[0];
    if(output.data&&output.chunk){
        const size_t capacity=output.maxsize/8;
        const size_t frames=buffer->requested?std::min(uint64_t(capacity),buffer->requested):capacity;
        auto *samples=static_cast<float*>(output.data);
        auto read=w.read.load(std::memory_order_relaxed);
        const auto write=w.write.load(std::memory_order_acquire);
        for(size_t i=0;i<frames;++i){
            if(write-read>=2){samples[2*i]=w.ring[read%RingSize];samples[2*i+1]=w.ring[(read+1)%RingSize];read+=2;}
            else samples[2*i]=samples[2*i+1]=0;
        }
        w.read.store(read,std::memory_order_release);
        output.chunk->offset=0;output.chunk->stride=8;output.chunk->size=uint32_t(frames*8);
        buffer->size=frames;
    }
    pw_stream_queue_buffer(w.playback,buffer);
}
void onState(void *data,pw_stream_state,pw_stream_state state,const char*) {
    if(state==PW_STREAM_STATE_ERROR)fail(*static_cast<Worker*>(data));
}
void onGlobal(void *data,uint32_t id,uint32_t,const char *type,uint32_t,const spa_dict *props) {
    auto &w=*static_cast<Worker*>(data);
    if(w.ready||std::strcmp(type,PW_TYPE_INTERFACE_Node)!=0||!props)return;
    const auto *name=spa_dict_lookup(props,PW_KEY_NODE_NAME);
    if(!name||std::strcmp(name,"phosrack.equalizer")!=0)return;
    w.ready=true;std::printf("{\"ready\":true,\"objectId\":%u}\n",id);std::fflush(stdout);
}
void onCoreError(void *data,uint32_t id,int,int result,const char*) {
    if(id==PW_ID_CORE&&result<0)fail(*static_cast<Worker*>(data));
}
const pw_registry_events registryEvents{PW_VERSION_REGISTRY_EVENTS,onGlobal,nullptr};
const pw_core_events coreEvents=[] {pw_core_events e{};e.version=PW_VERSION_CORE_EVENTS;e.error=onCoreError;return e;}();
const pw_stream_events captureEvents=[] {pw_stream_events e{};e.version=PW_VERSION_STREAM_EVENTS;e.process=onCapture;e.state_changed=onState;return e;}();
const pw_stream_events playbackEvents=[] {pw_stream_events e{};e.version=PW_VERSION_STREAM_EVENTS;e.process=onPlayback;e.state_changed=onState;return e;}();
bool controls(Worker &w,const QJsonObject &object){
    const auto gains=object.value("gains").toArray();
    const double preamp=object.value("preamp").toDouble();
    if(gains.size()!=10||!std::isfinite(preamp))return false;
    for(int i=0;i<10;++i)if(!std::isfinite(gains[i].toDouble()))return false;
    for(int i=0;i<10;++i)w.gains[i].store(std::clamp(gains[i].toDouble(),-12.,12.),std::memory_order_relaxed);
    w.preamp.store(std::clamp(preamp,-12.,12.),std::memory_order_relaxed);return true;
}
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    if(app.arguments().size()!=2)return 2;
    QFile input(app.arguments()[1]);
    if(!input.open(QIODevice::ReadOnly))return 2;
    const auto config=QJsonDocument::fromJson(input.readAll()).object();
    const auto target=config.value("target").toString();
    if(target.isEmpty()||target.startsWith("phosrack.equalizer"))return 2;
    Worker worker;worker.app=&app;if(!controls(worker,config))return 2;
    pw_init(&argc,&argv);
    worker.loop=pw_thread_loop_new("phosrack-eq",nullptr);if(!worker.loop)return 3;
    worker.context=pw_context_new(pw_thread_loop_get_loop(worker.loop),nullptr,0);if(!worker.context)return 3;
    worker.core=pw_context_connect(worker.context,nullptr,0);if(!worker.core)return 3;
    pw_core_add_listener(worker.core,&worker.coreListener,&coreEvents,&worker);
    worker.registry=pw_core_get_registry(worker.core,PW_VERSION_REGISTRY,0);
    pw_registry_add_listener(worker.registry,&worker.registryListener,&registryEvents,&worker);
    const auto group=QByteArray("phosrack-eq-")+QByteArray::number(getpid());
    auto *captureProps=pw_properties_new(
        PW_KEY_NODE_NAME,"phosrack.equalizer",PW_KEY_NODE_DESCRIPTION,"Phosrack EQ",
        PW_KEY_MEDIA_CLASS,"Audio/Sink",PW_KEY_MEDIA_TYPE,"Audio",PW_KEY_MEDIA_CATEGORY,"Filter",
        PW_KEY_NODE_VIRTUAL,"true","priority.session","0",PW_KEY_NODE_GROUP,group.constData(),
        PW_KEY_NODE_LINK_GROUP,group.constData(),PW_KEY_NODE_LATENCY,"1024/48000",nullptr);
    auto *playbackProps=pw_properties_new(
        PW_KEY_NODE_NAME,"phosrack.equalizer.output",PW_KEY_MEDIA_CLASS,"Stream/Output/Audio",
        PW_KEY_MEDIA_TYPE,"Audio",PW_KEY_MEDIA_CATEGORY,"Playback",
        PW_KEY_TARGET_OBJECT,target.toUtf8().constData(),PW_KEY_NODE_GROUP,group.constData(),
        PW_KEY_NODE_LINK_GROUP,group.constData(),PW_KEY_NODE_LATENCY,"1024/48000",
        "node.dont-fallback","true","node.dont-reconnect","true",nullptr);
    worker.capture=pw_stream_new(worker.core,"Phosrack EQ",captureProps);
    worker.playback=pw_stream_new(worker.core,"Phosrack EQ Output",playbackProps);
    if(!worker.capture||!worker.playback)return 3;
    pw_stream_add_listener(worker.capture,&worker.captureListener,&captureEvents,&worker);
    pw_stream_add_listener(worker.playback,&worker.playbackListener,&playbackEvents,&worker);
    uint8_t buffer[1024];spa_pod_builder builder=SPA_POD_BUILDER_INIT(buffer,sizeof(buffer));
    spa_audio_info_raw audio{};audio.format=SPA_AUDIO_FORMAT_F32;audio.rate=48000;audio.channels=2;
    audio.position[0]=SPA_AUDIO_CHANNEL_FL;audio.position[1]=SPA_AUDIO_CHANNEL_FR;
    const spa_pod *format=spa_format_audio_raw_build(&builder,SPA_PARAM_EnumFormat,&audio);
    const auto flags=pw_stream_flags(PW_STREAM_FLAG_MAP_BUFFERS|PW_STREAM_FLAG_RT_PROCESS);
    // Input is a virtual output device, not an autoconnecting capture client.
    // No microphone, default input or arbitrary capture target can be linked.
    if(pw_stream_connect(worker.capture,PW_DIRECTION_INPUT,PW_ID_ANY,flags,&format,1)<0)return 3;
    if(pw_stream_connect(worker.playback,PW_DIRECTION_OUTPUT,PW_ID_ANY,
        pw_stream_flags(flags|PW_STREAM_FLAG_AUTOCONNECT),&format,1)<0)return 3;
    if(pw_thread_loop_start(worker.loop)<0)return 3;
    QByteArray pending;QSocketNotifier notifier(STDIN_FILENO,QSocketNotifier::Read);
    QObject::connect(&notifier,&QSocketNotifier::activated,&app,[&]{
        char data[4096];const auto size=::read(STDIN_FILENO,data,sizeof(data));
        if(size<=0){notifier.setEnabled(false);app.quit();return;}
        pending.append(data,int(size));if(pending.size()>65536){pending.clear();return;}
        int newline=-1;
        while((newline=int(pending.indexOf('\n')))>=0){
            const auto object=QJsonDocument::fromJson(pending.left(newline)).object();pending.remove(0,newline+1);
            if(controls(worker,object)){std::printf("{\"applied\":true}\n");std::fflush(stdout);}
        }
    });
    std::signal(SIGTERM,interruptedSignal);std::signal(SIGINT,interruptedSignal);
    QTimer terminationPoll;terminationPoll.setInterval(50);
    QObject::connect(&terminationPoll,&QTimer::timeout,&app,[&]{if(interrupted)app.quit();});terminationPoll.start();
    const int result=app.exec();
    pw_thread_loop_stop(worker.loop);
    pw_stream_destroy(worker.capture);pw_stream_destroy(worker.playback);
    spa_hook_remove(&worker.registryListener);spa_hook_remove(&worker.coreListener);
    pw_proxy_destroy(reinterpret_cast<pw_proxy*>(worker.registry));
    pw_core_disconnect(worker.core);pw_context_destroy(worker.context);pw_thread_loop_destroy(worker.loop);pw_deinit();
    return worker.failure?worker.failure:result;
}
