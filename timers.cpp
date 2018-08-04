#include <iostream>
#include "types.hpp"
#include "timers.hpp"
#include "cpu.hpp"

namespace TIMERS {

    u32 lastTick;

    struct CT32 {
	u32 IR,
	    TCR,
	    TC,
	    PR,
	    PC,
	    MCR,
	    MR0,
	    MR1,
	    MR2,
	    MR3,
	    CCR,
	    CR0,
	    CR1,
	    CR2;	

	void tick( u32 delta ){
	    
	    if( !(TCR&1) )
		return;
	    
	    PC += delta;
	    
	    if( PC >= PR ){
		PC -= PR;
		TC++;
	    }
	    
	}

    } B1, B0;
    
    struct SysTick {

	u32 CSR, RVR, CVR, CALIB;
	
	void tick( u32 delta ){
	    
	    if( !(CSR & 1) ) return;
	    
	    // if( !(CSR & 0x4) ) delta >>= 1;
	    
	    CVR -= delta;
	    if( s32(CVR) < 0 ){
		CVR += RVR & 0xFFFFFF;
		CSR |= 1 << 16;
	    }

	    if( CPU::armIrqEnable && (CSR&(1<<16)) ){
		// std::cout << CVR << std::endl;		
		// CPU::echo = 0;
		// CPU::echoRes = 0;
		CPU::interrupt(15);
	    }
	    
	}
	
    } sys;

    u32 setCVR( u32, u32, u32 ){
	sys.CSR &= ~(1<<16);
	return sys.RVR;
    }


    void update(){
	u32 delta = CPU::cpuTotalTicks - lastTick;
	if( !delta ) return;
	lastTick = CPU::cpuTotalTicks;
	sys.tick( delta );
	B0.tick( delta );
	B1.tick( delta );
    }

    template<CT32& ct>
    u32 writeTCR( u32 v, u32 ov, u32 addr ){
	if( v&2 ){
	    ct.TC = 0;
	    ct.PC = 0;
	}
	return v;
    }

    template<CT32& ct>
    u32 writeIR( u32 v, u32 ov, u32 addr ){
	return ov & ~v;
    }

    template<CT32& ct>
    u32 readTC( u32 v, u32 addr ){
	update();
	return ct.TC;
    }
    
    MMU::Register ct32b1Map[] = {
	MMUREGW(B1.IR, writeIR<B1> ),
	MMUREGW(B1.TCR, writeTCR<B1> ),
	MMUREGRm(B1.TC, readTC<B1>),
	MMUREG(B1.PR),
	MMUREG(B1.PC),
	MMUREG(B1.MCR),
	MMUREG(B1.MR0),
	MMUREG(B1.MR1),
	MMUREG(B1.MR2),
	MMUREG(B1.MR3),
	MMUREG(B1.CCR),
	MMUREG(B1.CR0),
	MMUREG(B1.CR1),
	MMUREG(B1.CR2)
    };

    MMU::Register ct32b0Map[] = {
	MMUREGW(B0.IR, writeIR<B0> ),
	MMUREGW(B0.TCR, writeTCR<B0> ),
	MMUREGR(B0.TC, readTC<B0>),
	MMUREG(B0.PR),
	MMUREG(B0.PC),
	MMUREG(B0.MCR),
	MMUREG(B0.MR0),
	MMUREG(B0.MR1),
	MMUREG(B0.MR2),
	MMUREG(B0.MR3),
	MMUREG(B0.CCR),
	MMUREG(B0.CR0),
	MMUREG(B0.CR1),
	MMUREG(B0.CR2)
    };
    
    MMU::Register systickMap[] = {
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREG(sys.CSR),
	MMUREG(sys.RVR),
	MMUREGW(sys.CVR, setCVR),
	MMUREG(sys.CALIB)
    };

    MMU::Layout ct32b1Layout = {
	0x40018000,
	sizeof(ct32b1Map) / sizeof(ct32b1Map[0]),
	ct32b1Map
    };


    MMU::Layout ct32b0Layout = {
	0x40014000,
	sizeof(ct32b0Map) / sizeof(ct32b0Map[0]),
	ct32b0Map
    };
    
    MMU::Layout systickLayout = {
	0xE000E000,
	sizeof(systickMap) / sizeof(systickMap[0]),
	systickMap
    };


    void init(){
	lastTick = 0;
	sys.CALIB = 4;
	
/*
	for( u32 i=0; i<sizeof(systickMap)/sizeof(systickMap[0]); ++i ){
	    std::cout << systickMap[i].name
		      << " "
		      << std::hex
		      << (systickLayout.base+(i*4))
		      << std::endl;
	}
/* */
    }
    
}

