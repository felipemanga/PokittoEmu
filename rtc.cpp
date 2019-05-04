#include <iostream>
#include <chrono>
#include "types.hpp"
#include "rtc.hpp"
#include "cpu.hpp"
#include "sys.hpp"

namespace RTC {

    u32 RTC_EN;
    u32 CTRL, MATCH, COUNT, WAKE;

    u32 writeCTRL( u32 v, u32 ov, u32 addr ){
        RTC_EN = !(v&1) && (v&(1<<7)) && (SYS::SYSAHBCLKCTRL&(1<<30));
        return v;
    }

    u32 readCOUNT( u32 v, u32 addr ){
        if( !RTC_EN ){
            return v;
        }

	using namespace std::chrono;
        v = duration_cast< seconds >(
	    system_clock::now().time_since_epoch()
	).count();
        
        return v;
    }

    MMU::Register map[] = {
        MMUREGW( CTRL, writeCTRL ),
        MMUREG( MATCH ),
        MMUREGR( COUNT, readCOUNT ),
        MMUREG( WAKE )
    };
    
    MMU::Layout layout = {
        0x40024000,
        sizeof(map) / sizeof(map[0]),
        map
    };

    void init(){
	using namespace std::chrono;
        CTRL = 7;
        MATCH = 0xFFFF;
        COUNT = duration_cast< seconds >(
	    system_clock::now().time_since_epoch()
	).count();
    }
    
}
