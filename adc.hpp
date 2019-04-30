#pragma once

#include "mmu.hpp"

namespace ADC {

    void init();

    extern MMU::Layout layout;
    extern u32 CTRL,
	SEQA_CTRL,
	SEQB_CTRL,
	SEQA_GDAT,
	SEQB_GDAT,
	DAT0,
	DAT1,
	DAT2,
	DAT3,
	DAT4,
	DAT5,
	DAT6,
	DAT7,
	DAT8,
	DAT9,
	DAT10,
	DAT11;

}
