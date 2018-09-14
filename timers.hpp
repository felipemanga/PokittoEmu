#pragma once

#include "mmu.hpp"

namespace TIMERS {

    void init();
    u32 update();

    extern MMU::Layout ct32b0Layout, ct32b1Layout, systickLayout;


}
