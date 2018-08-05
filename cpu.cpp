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
#include <algorithm>

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

    u32 cpuTotalTicks;
    u32 cpuNextEvent;

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

// Waitstates when accessing data
    int dataTicksAccess16(u32 address) // DATA 8/16bits NON SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;
	  int value =  memoryWait[addr];

	  if ((addr>=0x08) || (addr < 0x02))
	  {
	  busPrefetchCount=0;
	  busPrefetch=false;
	  }
	  else if (busPrefetch)
	  {
	  int waitState = value;
	  if (!waitState)
	  waitState = 1;
	  busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	  }

	  return value;
	*/
    }

    int dataTicksAccess32(u32 address) // DATA 32bits NON SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;
	  int value = memoryWait32[addr];

	  if ((addr>=0x08) || (addr < 0x02))
	  {
	  busPrefetchCount=0;
	  busPrefetch=false;
	  }
	  else if (busPrefetch)
	  {
	  int waitState = value;
	  if (!waitState)
	  waitState = 1;
	  busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	  }

	  return value;
	*/
    }

    int dataTicksAccessSeq16(u32 address)// DATA 8/16bits SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;
	  int value = memoryWaitSeq[addr];

	  if ((addr>=0x08) || (addr < 0x02))
	  {
	  busPrefetchCount=0;
	  busPrefetch=false;
	  }
	  else if (busPrefetch)
	  {
	  int waitState = value;
	  if (!waitState)
	  waitState = 1;
	  busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	  }

	  return value;
	*/
    }

    int dataTicksAccessSeq32(u32 address)// DATA 32bits SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;
	  int value =  memoryWaitSeq32[addr];

	  if ((addr>=0x08) || (addr < 0x02))
	  {
	  busPrefetchCount=0;
	  busPrefetch=false;
	  }
	  else if (busPrefetch)
	  {
	  int waitState = value;
	  if (!waitState)
	  waitState = 1;
	  busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
	  }

	  return value;
	*/
    }


// Waitstates when executing opcode
    int codeTicksAccess16(u32 address) // THUMB NON SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;

	  if ((addr>=0x08) && (addr<=0x0D))
	  {
	  if (busPrefetchCount&0x1)
	  {
	  if (busPrefetchCount&0x2)
	  {
	  busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
	  return 0;
	  }
	  busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
	  return memoryWaitSeq[addr]-1;
	  }
	  else
	  {
	  busPrefetchCount=0;
	  return memoryWait[addr];
	  }
	  }
	  else
	  {
	  busPrefetchCount = 0;
	  return memoryWait[addr];
	  }
	*/
    }

    int codeTicksAccess32(u32 address) // ARM NON SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;

	  if ((addr>=0x08) && (addr<=0x0D))
	  {
	  if (busPrefetchCount&0x1)
	  {
	  if (busPrefetchCount&0x2)
	  {
	  busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
	  return 0;
	  }
	  busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
	  return memoryWaitSeq[addr] - 1;
	  }
	  else
	  {
	  busPrefetchCount = 0;
	  return memoryWait32[addr];
	  }
	  }
	  else
	  {
	  busPrefetchCount = 0;
	  return memoryWait32[addr];
	  }
	*/
    }

    int codeTicksAccessSeq16(u32 address) // THUMB SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;

	  if ((addr>=0x08) && (addr<=0x0D))
	  {
	  if (busPrefetchCount&0x1)
	  {
	  busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
	  return 0;
	  }
	  else
	  if (busPrefetchCount>0xFF)
	  {
	  busPrefetchCount=0;
	  return memoryWait[addr];
	  }
	  else
	  return memoryWaitSeq[addr];
	  }
	  else
	  {
	  busPrefetchCount = 0;
	  return memoryWaitSeq[addr];
	  }
	*/
    }

    int codeTicksAccessSeq32(u32 address) // ARM SEQ
    {
	return 0;
	/*
	  int addr = (address>>24)&15;

	  if ((addr>=0x08) && (addr<=0x0D))
	  {
	  if (busPrefetchCount&0x1)
	  {
	  if (busPrefetchCount&0x2)
	  {
	  busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
	  return 0;
	  }
	  busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
	  return memoryWaitSeq[addr];
	  }
	  else
	  if (busPrefetchCount>0xFF)
	  {
	  busPrefetchCount=0;
	  return memoryWait32[addr];
	  }
	  else
	  return memoryWaitSeq32[addr];
	  }
	  else
	  {
	  return memoryWaitSeq32[addr];
	  }
	*/
    }

    void CPUSwitchMode(int mode, bool saveState, bool breakLoop)
    {
	/*
	  if(armMode == mode)
	  return;

	  CPUUpdateCPSR();

	  switch (armMode)
	  {
	  case 0x10:
	  case 0x1F:
	  reg[R13_USR].I = reg[13].I;
	  reg[R14_USR].I = reg[14].I;
	  reg[17].I = reg[16].I;
	  break;
	  case 0x11:
	  CPUSwap(&reg[R8_FIQ].I, &reg[8].I);
	  CPUSwap(&reg[R9_FIQ].I, &reg[9].I);
	  CPUSwap(&reg[R10_FIQ].I, &reg[10].I);
	  CPUSwap(&reg[R11_FIQ].I, &reg[11].I);
	  CPUSwap(&reg[R12_FIQ].I, &reg[12].I);
	  reg[R13_FIQ].I = reg[13].I;
	  reg[R14_FIQ].I = reg[14].I;
	  reg[SPSR_FIQ].I = reg[17].I;
	  break;
	  case 0x12:
	  reg[R13_IRQ].I  = reg[13].I;
	  reg[R14_IRQ].I  = reg[14].I;
	  reg[SPSR_IRQ].I =  reg[17].I;
	  break;
	  case 0x13:
	  reg[R13_SVC].I  = reg[13].I;
	  reg[R14_SVC].I  = reg[14].I;
	  reg[SPSR_SVC].I =  reg[17].I;
	  break;
	  case 0x17:
	  reg[R13_ABT].I  = reg[13].I;
	  reg[R14_ABT].I  = reg[14].I;
	  reg[SPSR_ABT].I =  reg[17].I;
	  break;
	  case 0x1b:
	  reg[R13_UND].I  = reg[13].I;
	  reg[R14_UND].I  = reg[14].I;
	  reg[SPSR_UND].I =  reg[17].I;
	  break;
	  }

	  u32 CPSR = reg[16].I;
	  u32 SPSR = reg[17].I;

	  switch (mode)
	  {
	  case 0x10:
	  case 0x1F:
	  reg[13].I = reg[R13_USR].I;
	  reg[14].I = reg[R14_USR].I;
	  reg[16].I = SPSR;
	  break;
	  case 0x11:
	  CPUSwap(&reg[8].I, &reg[R8_FIQ].I);
	  CPUSwap(&reg[9].I, &reg[R9_FIQ].I);
	  CPUSwap(&reg[10].I, &reg[R10_FIQ].I);
	  CPUSwap(&reg[11].I, &reg[R11_FIQ].I);
	  CPUSwap(&reg[12].I, &reg[R12_FIQ].I);
	  reg[13].I = reg[R13_FIQ].I;
	  reg[14].I = reg[R14_FIQ].I;
	  if (saveState)
	  reg[17].I = CPSR;
	  else
	  reg[17].I = reg[SPSR_FIQ].I;
	  break;
	  case 0x12:
	  reg[13].I = reg[R13_IRQ].I;
	  reg[14].I = reg[R14_IRQ].I;
	  reg[16].I = SPSR;
	  if (saveState)
	  reg[17].I = CPSR;
	  else
	  reg[17].I = reg[SPSR_IRQ].I;
	  break;
	  case 0x13:
	  reg[13].I = reg[R13_SVC].I;
	  reg[14].I = reg[R14_SVC].I;
	  reg[16].I = SPSR;
	  if (saveState)
	  reg[17].I = CPSR;
	  else
	  reg[17].I = reg[SPSR_SVC].I;
	  break;
	  case 0x17:
	  reg[13].I = reg[R13_ABT].I;
	  reg[14].I = reg[R14_ABT].I;
	  reg[16].I = SPSR;
	  if (saveState)
	  reg[17].I = CPSR;
	  else
	  reg[17].I = reg[SPSR_ABT].I;
	  break;
	  case 0x1b:
	  reg[13].I = reg[R13_UND].I;
	  reg[14].I = reg[R14_UND].I;
	  reg[16].I = SPSR;
	  if (saveState)
	  reg[17].I = CPSR;
	  else
	  reg[17].I = reg[SPSR_UND].I;
	  break;
	  default:
	  g_message("Unsupported ARM mode %02x", mode);
	  break;
	  }
	  armMode = mode;
	  CPUUpdateFlags(breakLoop);
	  CPUUpdateCPSR();
	*/
    }

    void CPUSwitchMode(int mode, bool saveState)
    {
	CPUSwitchMode(mode, saveState, true);
    }

    void CPUUndefinedException()
    {
	u32 PC = reg[15].I;
	CPUSwitchMode(0x1b, true, false);
	reg[14].I = PC - 2;
	reg[15].I = 0x04;
	armIrqEnable = false;
	armNextPC = 0x04;
	THUMB_PREFETCH();
	reg[15].I += 2;
    }

    void CPUSoftwareInterrupt()
    {
	u32 PC = reg[15].I;
	CPUSwitchMode(0x13, true, false);
	reg[14].I = PC - 2;
	reg[15].I = 0x08;
	armIrqEnable = false;
	armNextPC = 0x08;
	THUMB_PREFETCH();
	reg[15].I += 2;
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
