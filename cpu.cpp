#include <algorithm>
#include <iostream>
#include "cpu.hpp"
// #include "GBA.h"
// #include "Globals.h"
#include "mmu.hpp"
// #include "../common/Settings.h"
#include "sys.hpp"
#include "iocon.hpp"
#include "adc.hpp"
#include "gpio.hpp"
#include "timers.hpp"
#include "sct.hpp"
#include "usart.hpp"
#include "gdb.hpp"

namespace CPU
{

    u32 cpuPrefetch[2];
    u8 cpuBitsSet[256];

    reg_pair reg[45];
    bool N_FLAG = 0;
    bool C_FLAG = 0;
    bool Z_FLAG = 0;
    bool V_FLAG = 0;

    bool armIrqEnable = true;
    u32 armNextPC = 0x00000000;
    u32 ADDRESS, PREVADDRESS;
    int armMode = 0x1f;

    bool busPrefetch = false; //TODO: never read ?
    bool busPrefetchEnable = false;
    u32 busPrefetchCount = 0;

    u64 cpuTotalTicks;
    u64 cpuNextEvent;

    void init()
    {
	for (int i = 0; i < 256; i++)
	{
	    int count = 0;
	    for (int j = 0; j < 8; j++)
		if (i & (1 << j))
		    count++;

	    cpuBitsSet[i] = count;
	}
    }

    void reset()
    {
	cpuTotalTicks = 0;
	SYS::init();
	IOCON::init();
	ADC::init();
	GPIO::init();
	TIMERS::init();
	SCT::init();
	USART::init();

	// clean registers
	for (int i = 0; i < 45; i++)
	    reg[i].I = 0;

	reg[15].I = MMU::read32( SYS::VTOR+4 );
	reg[14].I = ~0;
	reg[13].I = MMU::read32( SYS::VTOR );
	armIrqEnable = true;

	C_FLAG = false;
	V_FLAG = false;
	N_FLAG = false;
	Z_FLAG = false;

	CPUUpdateCPSR();

	armNextPC = reg[15].I;
	reg[15].I += 2;

	THUMB_PREFETCH();
    }

#ifdef WORDS_BIGENDIAN
    static void CPUSwap(volatile u32 *a, volatile u32 *b)
    {
	volatile u32 c = *b;
	*b = *a;
	*a = c;
    }
#else
    static void CPUSwap(u32 *a, u32 *b)
    {
	u32 c = *b;
	*b = *a;
	*a = c;
    }
#endif


    void CPUSwitchMode(int mode, bool saveState, bool breakLoop)
    {
    }

    void CPUSwitchMode(int mode, bool saveState)
    {
	CPUSwitchMode(mode, saveState, true);
    }

    void CPUUndefinedException()
    {
	if( GDB::connected() )
	    GDB::interrupt();
	else
	    reg[15].I += 2;
    }

    void CPUSoftwareInterrupt()
    {
    }

    void CPUSoftwareInterrupt(int comment)
    {

#ifdef GBA_LOGGING
	if (settings_log_channel_enabled(LOG_SWI))
	{
	    g_message("SWI: %08x at %08x (0x%08x,0x%08x,0x%08x,VCOUNT = %2d)\n", comment,
		      armState ? armNextPC - 4: armNextPC -2,
		      reg[0].I,
		      reg[1].I,
		      reg[2].I,
		      VCOUNT);
	}
#endif
	CPUSoftwareInterrupt();
    }

    void CPUUpdateCPSR()
    {
	u32 CPSR = reg[16].I & 0x40;
	if (N_FLAG)
	    CPSR |= 0x80000000;
	if (Z_FLAG)
	    CPSR |= 0x40000000;
	if (C_FLAG)
	    CPSR |= 0x20000000;
	if (V_FLAG)
	    CPSR |= 0x10000000;
	// if (!armIrqEnable) CPSR |= 0x80;
	CPSR |= 1<<24; // T-bit
	reg[16].I = CPSR;
    }

    void CPUUpdateFlags()
    {
	u32 CPSR = reg[16].I;
	N_FLAG = (CPSR & 0x80000000) ? true: false;
	Z_FLAG = (CPSR & 0x40000000) ? true: false;
	C_FLAG = (CPSR & 0x20000000) ? true: false;
	V_FLAG = (CPSR & 0x10000000) ? true: false;
	// armIrqEnable = (CPSR & 0x80) ? false : true;
    }

    void push( u32 v ){
	u32 addr = (reg[13].I -= 4);
	MMU::write32(addr, v);
	cpuTotalTicks++; 
    }


    u32 pop(){
	u32 ret = MMU::read32(reg[13].I);
	reg[13].I += 4;
	cpuTotalTicks++;
	return ret;
    }

    void exitInterrupt(){
	reg[0].I = pop();
	reg[1].I = pop();
	reg[2].I = pop();
	reg[3].I = pop();
	reg[12].I = pop();
	reg[14].I = pop();
	reg[15].I = pop() - 2;
	reg[16].I = pop();
	armIrqEnable = true;
	CPUUpdateFlags();
	armNextPC = reg[15].I;
	reg[15].I += 2;
	THUMB_PREFETCH();
    }
    
    void interrupt( u32 id )
    {
	CPUUpdateCPSR();
	push( reg[16].I );
	push( reg[15].I );
	push( reg[14].I );
	push( reg[12].I );
	push( reg[3].I );
	push( reg[2].I );
	push( reg[1].I );
	push( reg[0].I );
	reg[15].I = MMU::read32(SYS::VTOR+(id<<2));
	reg[14].I = 0xFFFFFFF9;
	armIrqEnable = false;
	
	armNextPC = reg[15].I;
	reg[15].I += 2;
	THUMB_PREFETCH();
    }

    void enableBusPrefetch(bool enable)
    {
    }

} // namespace CPU
