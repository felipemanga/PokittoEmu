#pragma once

#include "types.hpp"
#include "mmu.hpp"
// #include "Globals.h" //TODO: Remove

namespace CPU
{

    union reg_pair
    {
	struct
	{
#ifdef WORDS_BIGENDIAN
	    u8 B3;
	    u8 B2;
	    u8 B1;
	    u8 B0;
#else
	    u8 B0;
	    u8 B1;
	    u8 B2;
	    u8 B3;
#endif
	} B;
	struct
	{
#ifdef WORDS_BIGENDIAN
	    u16 W1;
	    u16 W0;
#else
	    u16 W0;
	    u16 W1;
#endif
	} W;
#ifdef WORDS_BIGENDIAN
	volatile u32 I;
#else
	u32 I;
#endif
    };

    extern reg_pair reg[45];

    extern bool N_FLAG;
    extern bool C_FLAG;
    extern bool Z_FLAG;
    extern bool V_FLAG;

// extern bool armState;
    extern bool armIrqEnable;
    extern u32 armNextPC;
    extern u32 ADDRESS, PREVADDRESS, OPCODE;
    extern int armMode;
    extern bool logops;

    extern u32 cpuPrefetch[2];
    extern u8 cpuBitsSet[256];

    extern u64 cpuTotalTicks;
    extern u64 cpuNextEvent;
    extern bool holdState;

    extern u32 echo, echoRes;
    
    void init();
    void reset();
    void interrupt(u32);
    void exitInterrupt();
    void enableBusPrefetch(bool enable);

    int thumbExecute();
    
    void CPUSwitchMode(int mode, bool saveState);
    void CPUSwitchMode(int mode, bool saveState, bool breakLoop);
    void CPUUpdateCPSR();
    void CPUUpdateFlags(bool breakLoop);
    void CPUUpdateFlags();
    void CPUUndefinedException();
    void CPUSoftwareInterrupt();
    void CPUSoftwareInterrupt(int comment);

    inline void THUMB_PREFETCH()
    {
	if( (armNextPC&~1) == 0xFFFFFFF8 )
	    exitInterrupt();
	
	cpuPrefetch[0] = MMU::exec16(armNextPC);
	cpuPrefetch[1] = MMU::exec16(armNextPC + 2);
    }

    inline void THUMB_PREFETCH_NEXT()
    {
	cpuPrefetch[1] = MMU::exec16(armNextPC+2);
    }

} // namespace CPU
