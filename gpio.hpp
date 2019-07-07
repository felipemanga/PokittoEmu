#pragma once

#include "mmu.hpp"

namespace GPIO {

    void init();
    void update();
    void input( u32 pinId, u32 bit, u32 val );
    extern MMU::Layout byteLayout, wordLayout, mainLayout, flagLayout;

    extern u32 POUT0,
	POUT1,
	POUT2;

}
