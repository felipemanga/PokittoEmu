#if !defined(__EMSCRIPTEN__)

#include <iostream>
#include <cstdio>
#include <numeric>
#include <SDL2/SDL.h>
#include <fstream>

#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

#include "types.hpp"
#include "cpu.hpp"

extern volatile bool hasQuit;

namespace PROF {
    u32 hits[ sizeof(MMU::flash)>>1 ];
    bool dumpOnExit = true;

    void init( bool _hitCaller ){
    }
}

#endif
