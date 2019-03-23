#pragma once

#include <unordered_map>
#include <thread>
#include <mutex>
#include <chrono>
#include <functional>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "types.hpp"
#include "initerror.hpp"

using namespace std::chrono_literals;

extern uint8_t rgba[220*176*4];

class SDL
{
    SDL_Window * m_window;
    SDL_Renderer * m_renderer;
    SDL_Surface *screen;

    std::unordered_map<int, SDL_Joystick*> joysticks;
    using EventHandler = std::function<void()>; // void (*)();
    std::vector< EventHandler > eventHandlers;
    std::mutex eventmut;

    u8 *screenpixels;
    u32 gifNum = 0;
    u32 delay = 2;
    bool recording = false;
    bool canTakeScreenshot = false;
    
#ifndef __EMSCRIPTEN__
    std::mutex gifmut;
#endif

public:
    u32 screenshot;
    SDL_Surface *vscreen;

    SDL( Uint32 flags = 0 );
    void toggleRecording();
    virtual ~SDL();
    void draw();
    void savePNG();

    void checkEvents();
    void emitEvents();
};
