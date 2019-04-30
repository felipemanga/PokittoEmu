#include <iostream>
#include <cstdlib>
#include "types.hpp"
#include "adc.hpp"

namespace ADC {

    u32 CTRL,
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

    void init(){
	SEQA_GDAT = 0x80000000;
        DAT8 = DAT9 = 0x8000;
    }

    u32 readDAT1( u32, u32 ){
	return std::rand() & 0xFFF;
    }

    u32 setCTRL( u32 val, u32 oldVal, u32 addr ){
	return 0;	
    }

    MMU::Register map[] = {
	MMUREGW( CTRL, setCTRL ),
	MMUREGX(),
	MMUREG( SEQA_CTRL ),
	MMUREG( SEQB_CTRL ),
	MMUREGRO( SEQA_GDAT, MMU::defaultRead ),
	MMUREGRO( SEQB_GDAT, MMU::defaultRead ),
	MMUREGX(),
	MMUREGX(),
	MMUREG( DAT0 ),
	MMUREGRO( DAT1, readDAT1 ),
	MMUREG( DAT2 ),
	MMUREG( DAT3 ),
	MMUREG( DAT4 ),
	MMUREG( DAT5 ),
	MMUREG( DAT6 ),
	MMUREG( DAT7 ),
	MMUREG( DAT8 ),
	MMUREG( DAT9 ),
	MMUREG( DAT10 ),
	MMUREG( DAT11 )
    };

    MMU::Layout layout = {
	0x4001C000,
	sizeof(map) / sizeof(map[0]),
	map
    };
}

