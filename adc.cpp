#include <iostream>
#include "types.hpp"
#include "adc.hpp"

namespace ADC {

    u32 CTRL;

    void init(){
    }

    u32 setCTRL( u32 val, u32 oldVal, u32 addr ){
	return 0;	
    }

    MMU::Register map[] = {
	MMUREGW( CTRL, setCTRL ),
	MMUREGX(),
    };

    MMU::Layout layout = {
	0x4001C000,
	sizeof(map) / sizeof(map[0]),
	map
    };
}

