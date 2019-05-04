#pragma once

#include "mmu.hpp"

namespace RTC {
    extern u32 CTRL, MATCH, COUNT, WAKE;
    void init();
    u32 update();
    extern MMU::Layout layout;
}
