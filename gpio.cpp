#include <iostream>
#include "types.hpp"
#include "gpio.hpp"
#include "sys.hpp"
#include "screen.hpp"
#include "cpu.hpp"
#include "sd.hpp"

namespace GPIO {

    u32 ISEL,
	IENR,
	IENF,
	RISE,
	FALL,
	IST,
	PMCTRL,
	PMSRC,
	PMCFG;

    u32 PIN0,
	PIN1,
	PIN2,
	POUT0,
	POUT1,
	POUT2,
	MASK0,
	MASK1,
	MASK2,
	DIR0,
	DIR1,
	DIR2;

    u32 *PIN[] = {&PIN0, &PIN1, &PIN2};
    
    void input( u32 pinId, u32 bit, u32 val ){
	val = !!val;
	u32 old = (*PIN[pinId]>>bit) & 1;
	if( old == val ) return;
//	std::cout << "Setting " << pinId << "_" << bit << " = " << val << std::endl;
	*PIN[pinId] = (*PIN[pinId]&~(1<<bit)) | (val<<bit);

	u32 id = bit, f = 1;
	if( pinId == 1 ) id += 24;
	else if( pinId == 2 ) id += 56;

	if( val ){
	
	    if( SYS::PINTSEL0 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL1 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL2 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL3 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL4 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL5 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL6 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL7 == id && (IENR & f) ){
		IST |= f; RISE |= f;
	    }
	
	    f <<= 1;

	}else{

	    if( SYS::PINTSEL0 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL1 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL2 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL3 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL4 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL5 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL6 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;

	    if( SYS::PINTSEL7 == id && (IENF & f) ){
		IST |= f; FALL |= f;
	    }
	
	    f <<= 1;
	    
	}
	
    }

    void update(){
	if( IST && CPU::armIrqEnable ){
	    for( u32 f=0; f<8; ++f ){
		if( IST & (1<<f) ){
		    CPU::interrupt(16+f);
		    return;
		}
	    }
	}
    }
    
    template<u32 &pin> u32 readPin( u32, u32 ){
	return pin;
    }

    template<u32 &pin> u32 writePin( u32 v, u32 ov=0, u32 addr=0 ){
	return pin = v;
    }

    template<> u32 writePin<POUT0>( u32 v, u32 ov, u32 addr ){
	SD::enabled = !(v & (1<<7));
	return POUT0 = v;
    }

    template<> u32 writePin<POUT1>( u32 v, u32 ov, u32 addr ){
	if( !(POUT1 & (1<<12)) && (v & (1<<12)) )
	    SCREEN::LCDWrite( (POUT0>>2)&1, POUT2>>3 );
	if( !(POUT1 & 1) && (v&1) )
	    SCREEN::LCDReset();
	return POUT1 = v;
    }
    
    template<u32 &pin, u32 bit> u32 readByte( u32, u32 addr ){
	u32 off = bit + (addr&3);
	return pin>>off;
    }

    template<u32 &pin, u32 bit> u32 writeByte( u32 v, u32, u32 addr ){
	u32 off = bit + (addr&3);
	pin = (pin & ~(1<<off)) | (!!(v&(0xFF<<((8<<(addr&3))-8)))<<off);
	return v;
    }


    template<u32 &pin, u32 bit> u32 readWord( u32, u32 ){
	return pin>>bit;
    }

    template<u32 &pin, u32 bit> u32 writeWord( u32 v, u32, u32 ){
	writePin<pin>( (pin & ~(1<<bit)) | ((!!v)<<bit) );
	return v;
    }

    template<u32 &pin> u32 setPin( u32 v, u32, u32 ){
	return writePin<pin>( pin | v );
    }

    template<u32 &pin> u32 clearPin( u32 v, u32, u32 ){
	return writePin<pin>( pin & ~v, 0, 0 );
    }

    template<u32 &pin> u32 togglePin( u32 v, u32, u32 ){
	return writePin<pin>( pin ^ v );
    }
    
    template<u32 &pin, u32 &mask> u32 readMasked( u32, u32 ){
	return pin & ~mask;
    }

    template<u32 &pin, u32 &mask> u32 writeMasked( u32 v, u32, u32 ){
	return writePin<pin>( (pin&mask) | (v&~mask), 0, 0 );
    }

    MMU::Register flagMap[] = {
	MMUREG( ISEL ),
	MMUREG( IENR ),
	{
	    &MMU::_reserved,
	    "SIENR", [](u32,u32){ return IENR; },
	    [](u32 v, u32,u32){
		IENR |= v;
		return IENR;
	    }
	},
	{
	    &MMU::_reserved,
	    "CIENR", [](u32,u32){ return IENR; },
	    [](u32 v, u32,u32){
		IENR &= ~v;
		return IENR;
	    }
	},
	MMUREG(IENF),
	{
	    &MMU::_reserved,
	    "SIENF", [](u32,u32){ return IENF; },
	    [](u32 v, u32,u32){
		IENF |= v;
		return IENF;
	    }
	},
	{
	    &MMU::_reserved,
	    "CIENF", [](u32,u32){ return IENF; },
	    [](u32 v, u32,u32){
		IENF &= ~v;
		return IENF;
	    }
	},
	MMUREG( RISE ),
	MMUREG( FALL ),
	{
	    &IST,
	    "IST",
	    [](u32,u32){
		return IST;
	    },
	    [](u32 v, u32,u32){
		IST &= ~v;
		RISE &= ~v;
		FALL &= ~v;
		return IST;
	    }
	}

    };
    
#define BYTEREG(B,p,b)							\
    { &MMU::_reserved, "B"#B, &readByte<PIN##p,b>, &writeByte<POUT##p,b> }

#define WORDREG(B,p,b)							\
    { &MMU::_reserved, "W"#B, &readWord<PIN##p,b>, &writeWord<POUT##p,b> }

    MMU::Register byteMap[] = {
	BYTEREG(0,0,0),
	BYTEREG(4,0,4),
	BYTEREG(8,0,8),
	BYTEREG(12,0,12),
	BYTEREG(16,0,16),
	BYTEREG(20,0,20),
	MMUREGX(), MMUREGX(),
	BYTEREG(32,1,0),
	BYTEREG(36,1,4),
	BYTEREG(40,1,8),
	BYTEREG(44,1,12),
	BYTEREG(48,1,16),
	BYTEREG(52,1,20),
	BYTEREG(56,1,24),
	BYTEREG(60,1,28),
	BYTEREG(64,2,0),
	BYTEREG(68,2,4),
	BYTEREG(72,2,8),
	BYTEREG(76,2,12),
	BYTEREG(80,2,16),
	BYTEREG(84,2,20)
    };
    
    MMU::Register wordMap[] = {
	WORDREG(0,0,0),
	WORDREG(1,0,1),
	WORDREG(2,0,2),
	WORDREG(3,0,3),
	WORDREG(4,0,4),
	WORDREG(5,0,5),
	WORDREG(6,0,6),
	WORDREG(7,0,7),
	WORDREG(8,0,8),
	WORDREG(9,0,9),
	WORDREG(10,0,10),
	WORDREG(11,0,11),
	WORDREG(12,0,12),
	WORDREG(13,0,13),
	WORDREG(14,0,14),
	WORDREG(15,0,15),
	WORDREG(16,0,16),
	WORDREG(17,0,17),
	WORDREG(18,0,18),
	WORDREG(19,0,19),
	WORDREG(20,0,20),
	WORDREG(21,0,21),
	WORDREG(22,0,22),
	WORDREG(23,0,23),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	MMUREGX(),
	
	WORDREG(32,1,0),
	WORDREG(33,1,1),
	WORDREG(34,1,2),
	WORDREG(35,1,3),
	WORDREG(36,1,4),
	WORDREG(37,1,5),
	WORDREG(38,1,6),
	WORDREG(39,1,7),
	WORDREG(40,1,8),
	WORDREG(41,1,9),
	WORDREG(42,1,10),
	WORDREG(43,1,11),
	WORDREG(44,1,12),
	WORDREG(45,1,13),
	WORDREG(46,1,14),
	WORDREG(47,1,15),
	WORDREG(48,1,16),
	WORDREG(49,1,17),
	WORDREG(50,1,18),
	WORDREG(51,1,19),
	WORDREG(52,1,20),
	WORDREG(53,1,21),
	WORDREG(54,1,22),
	WORDREG(55,1,23),
	WORDREG(56,1,24),
	WORDREG(57,1,25),
	WORDREG(58,1,26),
	WORDREG(59,1,27),
	WORDREG(60,1,28),
	WORDREG(61,1,29),
	WORDREG(62,1,30),
	WORDREG(63,1,31),

	WORDREG(64,2,0),
	WORDREG(65,2,1),
	WORDREG(66,2,2),
	WORDREG(67,2,3),
	WORDREG(68,2,4),
	WORDREG(69,2,5),
	WORDREG(70,2,6),
	WORDREG(71,2,7),
	WORDREG(72,2,8),
	WORDREG(73,2,9),
	WORDREG(74,2,10),
	WORDREG(75,2,11),
	WORDREG(76,2,12),
	WORDREG(77,2,13),
	WORDREG(78,2,14),
	WORDREG(79,2,15),
	WORDREG(80,2,16),
	WORDREG(81,2,17),
	WORDREG(82,2,18),
	WORDREG(83,2,19),
	WORDREG(84,2,20),
	WORDREG(85,2,21),
	WORDREG(86,2,22),
	WORDREG(87,2,23),
	WORDREG(88,2,24),
	WORDREG(89,2,25),
	WORDREG(90,2,26),
	WORDREG(91,2,27),
	WORDREG(92,2,28),
	WORDREG(93,2,29),
	WORDREG(94,2,30),
	WORDREG(95,2,31),
    };

    MMU::Register mainMap[] = {
	MMUREG(DIR0),
	MMUREG(DIR1),
	MMUREG(DIR2),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),
	MMUREG(MASK0),
	MMUREG(MASK1),
	MMUREG(MASK2),
	MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGRW(PIN0, readPin<PIN0>, writePin<POUT0>),
	MMUREGRW(PIN1, readPin<PIN1>, writePin<POUT1>),
	MMUREGRW(PIN2, readPin<PIN2>, writePin<POUT2>),
	MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	{ 
	    &MMU::_reserved, 
	    "MPIN0", 
	    readMasked<PIN0,MASK0>, 
	    writeMasked<POUT0,MASK0>
	},
	{ 
	    &MMU::_reserved, 
	    "MPIN1", 
	    readMasked<PIN1,MASK1>, 
	    writeMasked<POUT1,MASK1>
	},
	{ 
	    &MMU::_reserved, 
	    "MPIN2", 
	    readMasked<PIN2,MASK2>, 
	    writeMasked<POUT2,MASK2>
	},
	MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	{ 
	    &MMU::_reserved, 
	    "SET0", 
	    readPin<PIN0>, 
	    setPin<POUT0>
	},
	{ 
	    &MMU::_reserved, 
	    "SET1", 
	    readPin<PIN1>, 
	    setPin<POUT1>
	},
	{ 
	    &MMU::_reserved, 
	    "SET2", 
	    readPin<PIN2>, 
	    setPin<POUT2>
	},
	MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	{ 
	    &MMU::_reserved, 
	    "CLR0", 
	    readPin<PIN0>, 
	    clearPin<POUT0>
	},
	{ 
	    &MMU::_reserved, 
	    "CLR1", 
	    readPin<PIN1>, 
	    clearPin<POUT1>
	},
	{ 
	    &MMU::_reserved, 
	    "CLR2", 
	    readPin<PIN2>, 
	    clearPin<POUT2>
	},
	MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	MMUREGX(),MMUREGX(),MMUREGX(),MMUREGX(),
	{ 
	    &MMU::_reserved, 
	    "TOGGLE0", 
	    readPin<PIN0>, 
	    togglePin<POUT0>
	},
	{ 
	    &MMU::_reserved, 
	    "TOGGLE1", 
	    readPin<PIN1>, 
	    togglePin<POUT1>
	},
	{ 
	    &MMU::_reserved, 
	    "TOGGLE2", 
	    readPin<PIN2>, 
	    togglePin<POUT2>
	},
    };
    
    MMU::Layout byteLayout = {
	0xA0000000,
	sizeof(byteMap) / sizeof(byteMap[0]),
	byteMap
    };

    MMU::Layout wordLayout = {
	0xA0001000,
	sizeof(wordMap) / sizeof(wordMap[0]),
	wordMap
    };

    MMU::Layout mainLayout = {
	0xA0002000,
	sizeof(mainMap) / sizeof(mainMap[0]),
	mainMap
    };

    MMU::Layout flagLayout = {
	0xA0004000,
	sizeof(flagMap) / sizeof(flagMap[0]),
	flagMap
    };

    void init(){
	/*
	for( u32 i=0; i<sizeof(flagMap)/sizeof(flagMap[0]); ++i ){
	    std::cout << flagMap[i].name
		      << " "
		      << std::hex
		      << (flagLayout.base+(i*4))
		      << std::endl;
	}
/* */
    }
    
}

