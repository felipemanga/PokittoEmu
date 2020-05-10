
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

extern volatile bool hasQuit;

#endif

#include "types.hpp"
#include "cpu.hpp"

namespace PROF {
    u32 hits[ sizeof(MMU::flash)>>1 ];
    bool dumpOnExit = true;

    void init( bool _hitCaller ){
    }
}
