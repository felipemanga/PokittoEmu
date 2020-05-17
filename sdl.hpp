#pragma once

#include <unordered_map>
#include <thread>
#ifndef __EMSCRIPTEN__
#include <mutex>
#endif
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
    SDL_Window * m_window = nullptr;
    SDL_Renderer * m_renderer = nullptr;
    SDL_Surface *screen = nullptr;

    std::unordered_map<int, SDL_Joystick*> joysticks;
    using EventHandler = std::function<void()>; // void (*)();
    std::vector< EventHandler > eventHandlers;

    u8 *screenpixels = nullptr;
    u32 gifNum = 0;
    u32 delay = 2;
    bool recording = false;
    bool canTakeScreenshot = false;
    
#ifndef __EMSCRIPTEN__
    std::mutex eventmut;
    std::mutex gifmut;
#endif

public:
    u32 screenshot = 0;
    SDL_Surface *vscreen = nullptr;
    SDL_Surface *vscreen2x = nullptr;
    bool useScale2X = false;

    SDL( Uint32 flags = 0 );
    void toggleRecording();
    virtual ~SDL();
    void draw();
    void savePNG();

    void checkEvents();
    void emitEvents();
};
