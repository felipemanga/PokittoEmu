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
    extern u32 ADDRESS, PREVADDRESS;
    extern int armMode;
    extern bool logops;

    extern bool busPrefetch;
    extern bool busPrefetchEnable;
    extern u32 busPrefetchCount;

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

// Waitstates when accessing data
    inline int dataTicksAccess16(u32 address) // DATA 8/16bits NON SEQ
    {
	return 0;
    }

    inline int dataTicksAccess32(u32 address) // DATA 32bits NON SEQ
    {
	return 0;
    }

    inline int dataTicksAccessSeq16(u32 address)// DATA 8/16bits SEQ
    {
	return 0;
    }

    inline int dataTicksAccessSeq32(u32 address)// DATA 32bits SEQ
    {
	return 0;
    }


// Waitstates when executing opcode
    inline int codeTicksAccess16(u32 address) // THUMB NON SEQ
    {
	return 0;
    }

    inline int codeTicksAccess32(u32 address) // ARM NON SEQ
    {
	return 0;
    }

    inline int codeTicksAccessSeq16(u32 address) // THUMB SEQ
    {
	return 0;
    }

    inline int codeTicksAccessSeq32(u32 address) // ARM SEQ
    {
	return 0;
    }

    
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
	
	cpuPrefetch[0] = MMU::read16(armNextPC);
	cpuPrefetch[1] = MMU::read16(armNextPC + 2);
    }

    inline void THUMB_PREFETCH_NEXT()
    {
	cpuPrefetch[1] = MMU::read16(armNextPC+2);
    }

} // namespace CPU
