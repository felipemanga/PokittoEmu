#include "types.hpp"
#include "iocon.hpp"

namespace IOCON {

    u32 RESET_PIO0_0;

    void init(){
    }

    MMU::Register map[] = {
	MMUREG( RESET_PIO0_0 )
    };

    MMU::Layout layout = {
	0x40044000,
	sizeof(map) / sizeof(map[0]),
	map
    };
}

