#include <iostream>
#include "types.hpp"
#include "audio.hpp"
#include "gpio.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <thread>


u8 ring[4096];
volatile u32 start=0, end=0;
u32 b = 128;

static void audioCallback( void *data, unsigned char *stream, int len ){
    for( int i=0; i<len; ++i ){
        if( start == end ){
            for( ; i<len; ++i ){
                stream[i] = b;
            }
            return;
        }
        stream[i] = b = ring[start];
        start = (start + 1) & 0xFFF;
    }
}

namespace AUDIO {
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    SDL_AudioDeviceID dev;

    void write(u8 data){
        ring[end] = data;
        end = (end+1) & 0xFFF;
        if( start == end ){
            start = (start+1) & 0xFFF;
        }
    }

    bool init(u32 hz){
        SDL_zero(want);
        want.freq = hz;
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
    }
}
