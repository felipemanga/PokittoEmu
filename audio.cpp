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

    void update(){
        if(CPU::cpuTotalTicks - prevTicks > 1000)
            write(prevData);
    }
    
    void write(u8 data){
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
