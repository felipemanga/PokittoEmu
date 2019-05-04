#include <cstring>
#include "types.hpp"
#include "cpu.hpp"
#include "sys.hpp"
#include "iocon.hpp"
#include "adc.hpp"
#include "gpio.hpp"
#include "timers.hpp"
#include "usart.hpp"
#include "rtc.hpp"
#include "sct.hpp"
#include "spi.hpp"
#include <iostream>
#include "gdb.hpp"

extern u32 verbose;
namespace MMU
{
    bool mute = true;

    u32 ignoreBadWrites;

    u8 flash[0x40000];
    u8 sram[0x8000];
    u8 sram1[0x800];
    u8 usbsram[0x800];
    u8 eeprom[0x1000];

    struct ROM_t {

	struct {
	    u32 usbdApiBase; /*!< USBD API function table base address */
	    u32 reserved0; /*!< Reserved */
	    u32 reserved1; /*!< Reserved */
	    u32 pPWRD; /*!< Power API function table base address */
	    u32 divApiBase; /*!< Divider API function table base address */
	    u32 pI2CD; /*!< I2C driver API function table base address */
	    u32 pDMAD; /*!< DMA driver API function table base address */
	    u32 reserved2; /*!< Reserved */
	    u32 reserved3; /*!< Reserved */
	    u32 pUARTND; /*!< USART 1/2/3/4 driver API function table base address */
	    u32 reserved4; /*!< Reserved */
	    u32 pUART0D; /*!< USART 0 driver API function table base address */
	    
	} LPC_ROM_API;

	struct {
	    u32 sidiv;
	    u32 uidiv;
	    u32 sidivmod;
	    u32 uidivmod;
	} DIV_API;
	
    } romstruct = {
	{
	    0x30000000, // usbdApiBase
	    0x00000000, // reserved0
	    0x00000000, // reserved1
	    0x30000000, // API_T *pPWRD
	    0x30000030, // DIV_API_T *divApiBase
	    0x30000000, // I2CD_API_T *pI2CD
	    0x30000000, // DMAD_API_T *pDMAD
	    0x30000000, // u32 reserved2
	    0x30000000, // u32 reserved3
	    0x30000000, // UARTD_API_T *pUARTND
	    0x30000000, // u32 reserved4
	    0x30000000, // UARTD_API_T *pUART0D
	},

	{
	    0x1fff1ff3,
	    0x1fff1ff5,
	    0x1fff1ff7,
	    0x1fff1ff9
	}
    };

    u8 rom[ sizeof(romstruct) ];

    template< u8* buffer, u32 base, u32 size, typename retType > 
    retType readBuffer( u32 addr ){
	if( addr < base ){
	    if( verbose ){
		std::cout << "Read OOB " << std::hex << addr
			  << " on PC="
			  << CPU::ADDRESS
			  << " base = " << base
			  << std::endl;
	    }
	    return ~0;
	}

	if( addr == 0x1fff1ff8 )
	    return retType(0x30000000);

	addr -= base;

	if( addr >= size ){
	    if( verbose ){
		std::cout << "Read OOB " << std::hex << addr
			  << " on PC="
			  << CPU::ADDRESS
			  << " base = " << base
			  << std::endl;
	    }
	    return ~0;
	}

	return *((retType *)&buffer[addr]);
    }

    u32 warnaddr;
    template< typename valType > 
    void writeROR( u32 addr, valType value ){
	if( verbose ){
	    std::cout << " Write to read-only region: "
		      << std::hex << addr
		      << " = " << value
		      << " on PC="
		      << CPU::ADDRESS
		      << std::endl;
	}

	if( ignoreBadWrites )
	    ignoreBadWrites--;
	else if( GDB::connected() )
	    GDB::interrupt();
	else if( addr < 0x10000000 ){
	    
	    CPU::reg[15].I -= 2;
	    if( warnaddr != CPU::reg[15].I ){
		std::cout << "Attempt to write to flash (0x"
			  << std::hex << addr
			  << ") on PC=0x"
			  << CPU::ADDRESS
			  << std::endl;
	    }
	    warnaddr = CPU::reg[15].I;
	    CPU::interrupt(3);
	}

    }

    template< u8* buffer, u32 base, u32 size, typename valType > 
    void writeBuffer( u32 addr, valType value ){
	if( addr < base ){
	    
	    // To-Do: raise interrupt
	    if( verbose ){
		std::cout << "OOB Write " << std::hex << addr
			  << " = " << u32(value)
			  << " on PC="
			  << CPU::ADDRESS
			  << std::endl;
	    }
	    
	    return;
	}

	addr -= base;

	if( addr >= size ){
	    
	    if( verbose ){
		std::cout << "OOB Write " << std::hex << addr
			  << " = " << u32(value)
			  << " on PC="
			  << CPU::ADDRESS
			  << std::endl;
	    }
	    
	    return;
	}

	*((valType *)&buffer[addr]) = value;
    }

    u32 defaultRead( u32 v, u32 addr ){
	if( verbose > 3 )
	    std::cout << "Read defreg "
		      << std::hex << addr
		      << " = " << v << std::endl;
	return v;
    }
    
    u32 defaultWrite( u32 v, u32 ov, u32 addr ){
	if( verbose > 3 )
	    std::cout << "Write defreg "
		      << std::hex << addr
		      << " = " << v << std::endl;
	return v;
    }
    u32 nopWrite( u32 v, u32 ov, u32 addr ){ return ov; }

    u32 dbg = 0x90;
    u32 _reserved;
    u32 readHole( u32, u32 addr ){
	// To-Do: raise interrupt
	if( verbose ){
	
	    std::cout << "Reading hole: "
		      << std::hex << addr
		      << " on PC="
		      << CPU::ADDRESS
		      << std::endl;

	    if( addr == 0x400440dc ){
		return dbg;
	    }
	}
	
	return 0;
    }

    u32 writeHole( u32 value, u32, u32 addr ){
	// To-Do: raise interrupt
	if( verbose ){
	
	    std::cout << "Writing hole: "
		      << std::hex << addr
		      << " = " << value
		      << " on PC="
		      << CPU::ADDRESS
		      << std::endl;
	    if( addr == 0x400440dc ){
		return dbg = value;
	    }
	    
	}
	
	return 0;
    }

    u32 dbgRead( u32 v, u32 addr ){
        /*
	std::cout << "DBG Read @ 0x"
		  << std::hex
		  << CPU::ADDRESS << ": 0x"
                  << addr
                  << " = "
		  << v
		  << " (" << std::dec
		  << v
		  << ")"
		  << std::endl;
        */
	// std::cout << "DBG Read @ : 0x" << std::hex << CPU::ADDRESS << std::endl;
	
	if( GDB::connected() )
	    GDB::interrupt();

	return v;
    }

    u32 dbgWrite( u32 value, u32 old, u32 addr ){
        if( value != old )
	std::cout << "DBG Write @ 0x"
		  << std::hex
		  << CPU::ADDRESS << ": 0x"
                  << addr
                  << " = "
		  << value
		  << " (" << std::dec
		  << value
		  << ")"
		  << std::endl;
	return value;
    }
    
    template< Layout &layout, typename valType >
    valType readRegister( u32 addr ){
	u32 idx = (addr - layout.base) >> 2;
	if( idx >= layout.length ){

	    // To-Do: raise interrupt
	    if( verbose ){
	    
		std::cout << "Error reading register "
			  << std::hex << addr
			  << std::endl;
		
	    }
	    
	    return 0;
	}
	
	u32 value = *layout.map[ idx ].value;
	auto read = layout.map[ idx ].read;
	
	if( read )
	    value = *layout.map[ idx ].value = (*read)( value, addr );

	if( addr&3 )
	    value = (value >> ((8<<(addr&0x3))-8)) & valType(~0);

	if( verbose > 2 ){
	    std::cout << "Read "
		      << layout.map[idx].name
		      << std::hex	    
		      << "(@"
		      << addr
		      << ") = " << value << " "
		      << " on PC="
		      << CPU::ADDRESS
		      << std::endl;
	}

	return value;
    }

    template< Layout &layout, typename valType >
    void writeRegister( u32 addr, valType value ){
	u32 idx = (addr - layout.base) >> 2;
	if( idx >= layout.length ){
	    if( verbose ){
		std::cout << "Error writing register "
			  << std::hex << addr
			  << " = " << value
			  << std::endl;
	    }
	    return; // to-do: raise exception?
	}

	if( verbose > 2 ){
	    std::cout << "REG WRITE "
		      << layout.map[idx].name
		      << std::hex	    
		      << "(@"
		      << addr
		      << ") = " << value << " "
		      << " on PC="
		      << CPU::ADDRESS
		      << std::endl;
	}

	auto write = layout.map[ idx ].write;
	u32 oldvalue = *layout.map[ idx ].value;
	value = (value&valType(~0)) << ((8<<(addr&3))-8);
	value |= oldvalue & ~(u32(valType(~0))<<((8<<(addr&3))-8) );
	
	if( write )
	    value = (*write)( value, oldvalue, addr );
	
	*layout.map[ idx ].value = value;
    }
    
    struct MemoryBank {
	u32 (*read32)( u32 );
	u16 (*read16)( u32 );
	u8  (*read8)( u32 );
	void (*write32)( u32, u32 );
	void (*write16)( u32, u16 );
	void (*write8)( u32, u8 );
    };

    template <MemoryBank *mb, u32 shift, u32 mask> u32 readMap32( u32 addr ){
	return (*mb[ (addr >> shift) & mask ].read32)( addr );
    }

    template <MemoryBank *mb, u32 shift, u32 mask> u16 readMap16( u32 addr ){
	return (*mb[ (addr >> shift) & mask ].read16)( addr );
    }

    template <MemoryBank *mb, u32 shift, u32 mask> u8 readMap8( u32 addr ){
	return (*mb[ (addr >> shift) & mask ].read8)( addr );
    }

    template <MemoryBank *mb, u32 shift, u32 mask> void writeMap32( u32 addr, u32 val ){
	(*mb[ (addr >> shift) & mask ].write32)( addr, val );
    }

    template <MemoryBank *mb, u32 shift, u32 mask> void writeMap16( u32 addr, u16 val ){
	(*mb[ (addr >> shift) & mask ].write16)( addr, val );
    }

    template <MemoryBank *mb, u32 shift, u32 mask> void writeMap8( u32 addr, u8 val ){
	(*mb[ (addr >> shift) & mask ].write8)( addr, val );
    }

    MemoryBank hiramMap[2] = {
	{
	    &readBuffer<  sram1, 0x20000000, sizeof(sram1), u32 >,
	    &readBuffer<  sram1, 0x20000000, sizeof(sram1), u16 >,
	    &readBuffer<  sram1, 0x20000000, sizeof(sram1), u8  >,
	    &writeBuffer< sram1, 0x20000000, sizeof(sram1), u32 >,
	    &writeBuffer< sram1, 0x20000000, sizeof(sram1), u16 >,
	    &writeBuffer< sram1, 0x20000000, sizeof(sram1), u8  >
	},
	{
	    &readBuffer<  usbsram, 0x20004000, sizeof(usbsram), u32 >,
	    &readBuffer<  usbsram, 0x20004000, sizeof(usbsram), u16 >,
	    &readBuffer<  usbsram, 0x20004000, sizeof(usbsram), u8  >,
	    &writeBuffer< usbsram, 0x20004000, sizeof(usbsram), u32 >,
	    &writeBuffer< usbsram, 0x20004000, sizeof(usbsram), u16 >,
	    &writeBuffer< usbsram, 0x20004000, sizeof(usbsram), u8  >
	}
    };

    MemoryBank voidBank = {
	&readBuffer< nullptr, 0, 0, u32 >,
	&readBuffer< nullptr, 0, 0, u16 >,
	&readBuffer< nullptr, 0, 0, u8 >,
	&writeROR<u32>,
	&writeROR<u16>,
	&writeROR<u8>
    };

    MemoryBank sysconBank = {
	&readRegister<  SYS::layout, u32 >,
	&readRegister<  SYS::layout, u16 >,
	&readRegister<  SYS::layout, u8  >,
	&writeRegister< SYS::layout, u32 >,
	&writeRegister< SYS::layout, u16 >,
	&writeRegister< SYS::layout, u8  >
    };

    MemoryBank ioconBank = {
	&readRegister<  IOCON::layout, u32 >,
	&readRegister<  IOCON::layout, u16 >,
	&readRegister<  IOCON::layout, u8  >,
	&writeRegister< IOCON::layout, u32 >,
	&writeRegister< IOCON::layout, u16 >,
	&writeRegister< IOCON::layout, u8  >
    };

    MemoryBank adcBank = {
	&readRegister<  ADC::layout, u32 >,
	&readRegister<  ADC::layout, u16 >,
	&readRegister<  ADC::layout, u8  >,
	&writeRegister< ADC::layout, u32 >,
	&writeRegister< ADC::layout, u16 >,
	&writeRegister< ADC::layout, u8  >
    };

    MemoryBank rtcBank = {
	&readRegister<  RTC::layout, u32 >,
	&readRegister<  RTC::layout, u16 >,
	&readRegister<  RTC::layout, u8  >,
	&writeRegister< RTC::layout, u32 >,
	&writeRegister< RTC::layout, u16 >,
	&writeRegister< RTC::layout, u8  >
    };

    MemoryBank ct32b1Bank = {
	&readRegister<  TIMERS::ct32b1Layout, u32 >,
	&readRegister<  TIMERS::ct32b1Layout, u16 >,
	&readRegister<  TIMERS::ct32b1Layout, u8  >,
	&writeRegister< TIMERS::ct32b1Layout, u32 >,
	&writeRegister< TIMERS::ct32b1Layout, u16 >,
	&writeRegister< TIMERS::ct32b1Layout, u8  >
    };

    MemoryBank ct32b0Bank = {
	&readRegister<  TIMERS::ct32b0Layout, u32 >,
	&readRegister<  TIMERS::ct32b0Layout, u16 >,
	&readRegister<  TIMERS::ct32b0Layout, u8  >,
	&writeRegister< TIMERS::ct32b0Layout, u32 >,
	&writeRegister< TIMERS::ct32b0Layout, u16 >,
	&writeRegister< TIMERS::ct32b0Layout, u8  >
    };

    MemoryBank usart0Bank = {
	&readRegister<  USART::usart0Layout, u32 >,
	&readRegister<  USART::usart0Layout, u16 >,
	&readRegister<  USART::usart0Layout, u8  >,
	&writeRegister< USART::usart0Layout, u32 >,
	&writeRegister< USART::usart0Layout, u16 >,
	&writeRegister< USART::usart0Layout, u8  >
    };

    MemoryBank spi0Bank = {
	&readRegister<  SPI::spi0Layout, u32 >,
	&readRegister<  SPI::spi0Layout, u16 >,
	&readRegister<  SPI::spi0Layout, u8  >,
	&writeRegister< SPI::spi0Layout, u32 >,
	&writeRegister< SPI::spi0Layout, u16 >,
	&writeRegister< SPI::spi0Layout, u8  >
    };

    MemoryBank spi1Bank = {
	&readRegister<  SPI::spi1Layout, u32 >,
	&readRegister<  SPI::spi1Layout, u16 >,
	&readRegister<  SPI::spi1Layout, u8  >,
	&writeRegister< SPI::spi1Layout, u32 >,
	&writeRegister< SPI::spi1Layout, u16 >,
	&writeRegister< SPI::spi1Layout, u8  >
    };
    
    MemoryBank gpioByteBank = {
	&readRegister<  GPIO::byteLayout, u32 >,
	&readRegister<  GPIO::byteLayout, u16 >,
	&readRegister<  GPIO::byteLayout, u8  >,
	&writeRegister< GPIO::byteLayout, u32 >,
	&writeRegister< GPIO::byteLayout, u16 >,
	&writeRegister< GPIO::byteLayout, u8  >
    };


    MemoryBank gpioWordBank = {
	&readRegister<  GPIO::wordLayout, u32 >,
	&readRegister<  GPIO::wordLayout, u16 >,
	&readRegister<  GPIO::wordLayout, u8  >,
	&writeRegister< GPIO::wordLayout, u32 >,
	&writeRegister< GPIO::wordLayout, u16 >,
	&writeRegister< GPIO::wordLayout, u8  >
    };


    MemoryBank gpioMainBank = {
	&readRegister<  GPIO::mainLayout, u32 >,
	&readRegister<  GPIO::mainLayout, u16 >,
	&readRegister<  GPIO::mainLayout, u8  >,
	&writeRegister< GPIO::mainLayout, u32 >,
	&writeRegister< GPIO::mainLayout, u16 >,
	&writeRegister< GPIO::mainLayout, u8  >
    };

    MemoryBank gpioFlagBank = {
	&readRegister<  GPIO::flagLayout, u32 >,
	&readRegister<  GPIO::flagLayout, u16 >,
	&readRegister<  GPIO::flagLayout, u8  >,
	&writeRegister< GPIO::flagLayout, u32 >,
	&writeRegister< GPIO::flagLayout, u16 >,
	&writeRegister< GPIO::flagLayout, u8  >
    };
    
    MemoryBank systickBank = {
	&readRegister<  TIMERS::systickLayout, u32 >,
	&readRegister<  TIMERS::systickLayout, u16 >,
	&readRegister<  TIMERS::systickLayout, u8  >,
	&writeRegister< TIMERS::systickLayout, u32 >,
	&writeRegister< TIMERS::systickLayout, u16 >,
	&writeRegister< TIMERS::systickLayout, u8  >
    };    
    
    MemoryBank apbMap[32] = {
	voidBank, // 0 - I2C
	voidBank, // 1 - WWDT
	usart0Bank, // 2 - USART0
	voidBank, // 3 - 16-bit counter 0
	voidBank, // 4 - 16-bit counter 1
	ct32b0Bank, // 5 - 32-bit counter 0
	ct32b1Bank, // 6 - 32-bit counter 1
	adcBank,  // 7 - ADC
	voidBank, // 8 - I2C1
	rtcBank,  // 9 - RTC
	voidBank, // 10 - DMA TRIGMUX
	voidBank, // 11 - Reserved
	voidBank, // 12 - Reserved
	voidBank, // 13 - Reserved
	voidBank, // 14 - PMU
	voidBank, // 15 - flash/eeprom controller
	spi0Bank, // 16 - SSP0
	ioconBank, // 17 - IOCON
	sysconBank, // 18 - SYSCON
	voidBank, // 19 - USART4
	voidBank, // 20 - Reserved
	voidBank, // 21 - Reserved
	spi1Bank, // 22 - SSP1
	voidBank, // 23 - GPIO Group0 Interrupt
	voidBank, // 24 - GPIO Group1 Interrupt
	voidBank, // 25 - Reserved
	voidBank, // 26 - Reserved
	voidBank, // 27 - USART1
	voidBank, // 28 - USART2
	voidBank, // 29 - USART3
	voidBank, // 30 - Reserved
	voidBank  // 31 - Reserved
    };


    MemoryBank sct0Bank = {
	&readRegister<  SCT::sct0Layout, u32 >,
	&readRegister<  SCT::sct0Layout, u16 >,
	&readRegister<  SCT::sct0Layout, u8  >,
	&writeRegister< SCT::sct0Layout, u32 >,
	&writeRegister< SCT::sct0Layout, u16 >,
	&writeRegister< SCT::sct0Layout, u8  >
    };

    MemoryBank sct1Bank = {
	&readRegister<  SCT::sct1Layout, u32 >,
	&readRegister<  SCT::sct1Layout, u16 >,
	&readRegister<  SCT::sct1Layout, u8  >,
	&writeRegister< SCT::sct1Layout, u32 >,
	&writeRegister< SCT::sct1Layout, u16 >,
	&writeRegister< SCT::sct1Layout, u8  >
    };

    MemoryBank cdsMap[16] = {
	voidBank, // 0 - Reserved
	voidBank, // 1 - Reserved
	voidBank, // 2 - Reserved
	voidBank, // 3 - Reserved
	voidBank, // 4 - Reserved
	voidBank, // 5 - Reserved
	sct0Bank, // 6 - Reserved
	sct1Bank, // 7 - Reserved
	voidBank, // 8 - Reserved
	voidBank, // 9 - Reserved
	voidBank, //10 - Reserved
	voidBank, //11 - Reserved
	voidBank, //12 - Reserved
	voidBank, //13 - Reserved
	voidBank, //14 - Reserved
	voidBank, //15 - Reserved
    };

    MemoryBank gpioMap[0x10] = {
	gpioByteBank,
	gpioWordBank,
	gpioMainBank,
	voidBank,
	gpioFlagBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank
    };

    MemoryBank ppbMap[0x10] = {
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	voidBank,
	systickBank,
	voidBank
    };

    MemoryBank memoryMap[0x10] = {

	{ // 0x0 - Flash
	    &readBuffer< flash, 0, sizeof(flash), u32 >,
	    &readBuffer< flash, 0, sizeof(flash), u16 >,
	    &readBuffer< flash, 0, sizeof(flash), u8 >,
	    &writeROR<u32>,
	    &writeROR<u16>,
	    &writeROR<u8>
	},

	{ // 0x1 - SRAM
	    &readBuffer<  sram, 0x10000000, sizeof(sram), u32 >,
	    &readBuffer<  sram, 0x10000000, sizeof(sram), u16 >,
	    &readBuffer<  sram, 0x10000000, sizeof(sram), u8  >,
	    &writeBuffer< sram, 0x10000000, sizeof(sram), u32 >,
	    &writeBuffer< sram, 0x10000000, sizeof(sram), u16 >,
	    &writeBuffer< sram, 0x10000000, sizeof(sram), u8  >
	},

	{ // 0x2 - SRAM1 & USBSRAM
	    &readMap32< hiramMap, 14, 1 >,
	    &readMap16< hiramMap, 14, 1 >,
	    &readMap8 < hiramMap, 14, 1 >,
	    &writeMap32< hiramMap, 14, 1 >,
	    &writeMap16< hiramMap, 14, 1 >,
	    &writeMap8 < hiramMap, 14, 1 >
	},


	{ // 0x3 - ROM
	    &readBuffer< rom, 0x30000000, sizeof(rom), u32 >,
	    &readBuffer< rom, 0x30000000, sizeof(rom), u16 >,
	    &readBuffer< rom, 0x30000000, sizeof(rom), u8 >,
	    &writeROR<u32>,
	    &writeROR<u16>,
	    &writeROR<u8>
	},

	{ // 0x4 - Periferals
	    &readMap32<  apbMap, 14, 0x1F >,
	    &readMap16<  apbMap, 14, 0x1F >,
	    &readMap8 <  apbMap, 14, 0x1F >,
	    &writeMap32< apbMap, 14, 0x1F >,
	    &writeMap16< apbMap, 14, 0x1F >,
	    &writeMap8 < apbMap, 14, 0x1F >
	},

	{ // 0x5 - CDS = CRC, DMA, SCTIMER/PWM, 
	    &readMap32<  cdsMap, 13, 0xF >,
	    &readMap16<  cdsMap, 13, 0xF >,
	    &readMap8 <  cdsMap, 13, 0xF >,
	    &writeMap32< cdsMap, 13, 0xF >,
	    &writeMap16< cdsMap, 13, 0xF >,
	    &writeMap8 < cdsMap, 13, 0xF >	    
	},
	
	voidBank, // 0x6
	voidBank, // 0x7
	voidBank, // 0x8
	voidBank, // 0x9
	
	{ // 0xA - GPIO
	    &readMap32<  gpioMap, 12, 0x7 >,
	    &readMap16<  gpioMap, 12, 0x7 >,
	    &readMap8 <  gpioMap, 12, 0x7 >,
	    &writeMap32< gpioMap, 12, 0x7 >,
	    &writeMap16< gpioMap, 12, 0x7 >,
	    &writeMap8 < gpioMap, 12, 0x7 >
	},
	
	voidBank, // 0xB
	voidBank, // 0xC
	voidBank, // 0xD
	{ // 0xE - PPB Private Periferal Bus
	    &readMap32<  ppbMap, 12, 0xF >,
	    &readMap16<  ppbMap, 12, 0xF >,
	    &readMap8 <  ppbMap, 12, 0xF >,
	    &writeMap32< ppbMap, 12, 0xF >,
	    &writeMap16< ppbMap, 12, 0xF >,
	    &writeMap8 < ppbMap, 12, 0xF >
	},
	voidBank  // 0xF
    };

    bool init(){
	memcpy( rom, &romstruct, sizeof(rom) );
	return true;
    }

    void uninit(){}

    u32 read32(u32 address){
	return (*memoryMap[ address>>28 ].read32)( address&~3 );
    }

    u32 read16(u32 address){
	return (*memoryMap[ address>>28 ].read16)( address&~1 );
    }

    s16 read16s(u32 address){
	return (*memoryMap[ address>>28 ].read16)( address&~1 );
    }

    u8 read8(u32 address){
	return (*memoryMap[ address>>28 ].read8)( address );
    }

    void write32(u32 address, u32 value){
	return (*memoryMap[ address>>28 ].write32)( address&~3, value );
    }

    void write16(u32 address, u16 value){
	return (*memoryMap[ address>>28 ].write16)( address&~1, value );
    }

    void write8(u32 address, u8 value){
	return (*memoryMap[ address>>28 ].write8)( address, value );
    }

} // namespace MMU
