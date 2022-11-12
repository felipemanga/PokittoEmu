#include <iostream>
#include "types.hpp"
#include "audio.hpp"
#include "gpio.hpp"
#include "cpu.hpp"
#include "sys.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <thread>
#include <atomic>
#include <fstream>

static constexpr u32 FREQ = 22050;
static constexpr f32 iFREQ = 1.0f / FREQ;
static constexpr u32 BUFFSIZE = 1<<15;
static constexpr u32 BUFFMASK = BUFFSIZE - 1;
static u8 dataRing[BUFFSIZE];
static f32 deltaRing[BUFFSIZE];
volatile std::atomic<u32> start=0, end=0, size=0;
static u32 b = 128;
bool audioPaused;
static SDL_AudioSpec want;
static SDL_AudioSpec have;

static void audioCallback( void *data, unsigned char *stream, int len ){
    f32 err = 0;
    for( int i=0; i<len; ++i ){
        if(size < len){
            for( ; i<len; ++i ){
                stream[i] = b;
            }
            return;
        }

        while(size){
            f32 d = (deltaRing[start] + err) - iFREQ;
            if(d > 0){
                deltaRing[start] = d;
                err = 0;
                break;
            }
            b = dataRing[start];
            err = -d;
            start = (start + 1) & BUFFMASK;
            size--;
        }

        stream[i] = b;
    }
}

namespace AUDIO {
    SDL_AudioDeviceID dev;
    u32 prevData = ~0;
    u64 prevTicks = 0;


    enum class HLEState {
        detect,
        disabled,
        enabled
    } hleState = HLEState::detect;
    u32 irqChecksum = 0;
    u32 irqAddress = 0;
    u8* libAudioBufferAddr = nullptr;
    u32* libAudioPlayheadAddr = nullptr;

    void update(){
        if(CPU::cpuTotalTicks - prevTicks > 1000)
            write(prevData);
    }
    
    void write(u8 data){
        if (hleState == HLEState::enabled)
            return;

        // if( data == prevData ){
        //     return;
        // }
        prevData = data;

        f32 clock = SYS::SYSPLLCTRL == 0x25 ? 72000000 : 45000000;
        f32 delta = f32(CPU::cpuTotalTicks - prevTicks) / clock;
        prevTicks = CPU::cpuTotalTicks;

        deltaRing[end] = delta;
        dataRing[end] = data;

        end = (end+1) & BUFFMASK;
        if( start == end ){
            start = (start+1) & BUFFMASK;
        } else {
            size++;
        }
    }

    u8* address(u32 v) {
        if (v < sizeof(MMU::flash))
            return MMU::flash + v;

        if (v >= 0x10000000 && v < 0x10000000 + sizeof(MMU::sram))
            return MMU::sram + v - 0x10000000;

        if (v >= 0x20000000 && v < 0x20000000 + sizeof(MMU::sram1))
            return MMU::sram1 + v - 0x20000000;

        if (v >= 0x20004000 && v < 0x20004000 + sizeof(MMU::usbsram))
            return MMU::usbsram + v - 0x20004000;

        return nullptr;
    }

    std::ofstream out{"log.txt"};

    void checkHLE(u32 rate) {
        auto timerIRQ = MMU::read32(SYS::VTOR+(34<<2));
        if (hleState != HLEState::detect) {
            if (irqAddress == timerIRQ)
                return;
        }

        hleState = HLEState::disabled;

        irqAddress = timerIRQ;
        irqChecksum = 0;
        for (u32 i = 0; i < 10; ++i)
            irqChecksum += MMU::read32(timerIRQ + (i << 2));

        if (irqChecksum != 0x32a90803)
            return;

        auto vbuffer = MMU::read32(timerIRQ + 0x70 - 1);
        auto vplay = MMU::read32(timerIRQ + 0x6c - 1);
        libAudioBufferAddr = address(vbuffer);
        libAudioPlayheadAddr = (u32*) address(vplay);

        if (!libAudioBufferAddr || !libAudioPlayheadAddr)
            return;

        hleState = HLEState::enabled;

        SDL_PauseAudioDevice(dev, 1);
        SDL_CloseAudioDevice(dev);

        SDL_AudioSpec wanted;
        SDL_zero(wanted);
        wanted.freq = (SYS::SYSPLLCTRL == 0x25 ? 72000000 : 45000000) / rate;
        wanted.format = AUDIO_U8;
        wanted.channels = 1;
        wanted.samples = 512;
        wanted.callback = +[](void*, u8* stream, int len){
            auto src = libAudioBufferAddr + 512 * ((libAudioPlayheadAddr[0] >> 9) & 1);
            memcpy(stream, src, len);
            // memset(src, 127, len);
        };

        dev = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
        SDL_PauseAudioDevice(dev, 0);
    }

    bool init(u32 hz){
        SDL_zero(want);
        want.freq = FREQ;
        want.format = AUDIO_U8;
        want.channels = 1;
        want.samples = 1024;
        want.callback = audioCallback;
        dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if( dev == 0 ){
            std::cerr << "Error initializing audio: " << dev << std::endl;
            return false;
        }

        SDL_PauseAudioDevice(dev, 0);

        atexit([](){
                SDL_CloseAudioDevice(dev);
            });

        return true;
    }
}
