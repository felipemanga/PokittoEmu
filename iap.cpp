#include <iostream>
#include "types.hpp"
#include "iap.hpp"
#include "mmu.hpp"

extern u32 verbose;

namespace IAP {

    void writeEEPROM( u32 r0, u32 r1, u32 r2, u32 r3 ){
	u32 eeAddress = MMU::read32( r0+4 );
	u32 buffAddress = MMU::read32( r0+8 );
	u32 byteCount = MMU::read32( r0+12 );

	for( u32 i=0; i<byteCount; ++i )
	    MMU::eeprom[ eeAddress+i ] = MMU::read8( buffAddress+i );
	
	MMU::write32( r1, 0 );
    }
	
    void readEEPROM( u32 r0, u32 r1, u32 r2, u32 r3 ){ // read EEPROM
	u32 eeAddress = MMU::read32( r0+4 );
	u32 buffAddress = MMU::read32( r0+8 );
	u32 byteCount = MMU::read32( r0+12 );
	// console.log("Read EEPROM to #", buffAddress.toString(16), byteCount.toString(16));
	for( u32 i=0; i<byteCount; ++i )
	    MMU::write8( buffAddress+i, MMU::eeprom[ eeAddress+i ]|0 );
	MMU::write32( r1, 0 );
    }

    void prewrrite( u32 r0, u32 r1, u32 r2, u32 r3 ){
	MMU::write32( r1, 0 );
	if( !verbose ) return;
	u32 startSector = MMU::read32( r0+4 );
	u32 endSector = MMU::read32( r0+8 );
	std::cout << "Prepare sector for write: "
		  << startSector
		  << " - "
		  << endSector
		  << std::endl;
    }

    void wrisector( u32 r0, u32 r1, u32 r2, u32 r3 ){
	u32 dst = MMU::read32( r0+4 );
	u32 src = MMU::read32( r0+8 );
	u32 len = MMU::read32( r0+12 );
	u32 cclk = MMU::read32( r0+16 );

	// std::cout << "CLK: " << std::hex << cclk << std::endl;

	if( dst + len > sizeof(MMU::flash) ){
	    std::cout << "ERROR: write beyond flash size" << std::endl;
	    len = sizeof(MMU::flash) - dst;
	}

	if( dst & 0xFF ){
	    std::cout << "ERROR: write not on 256-byte boundry" << std::endl;
	    dst &= ~0xFF;
	}

	for( u32 i=0; i<len; ++i ){
	    /*
	    if( MMU::flash[dst] != 0xFF )
		std::cout << "WARN: write to non-empty flash" << std::endl;
	    */
	    MMU::flash[dst++] = MMU::read8( src++ );
	}

	if( verbose )
	    std::cout << "Write sector"
		  << dst
		  << " "
		  << src
		  << " "
		  << len
		  << " "
		  << cclk
		  << std::endl;
	MMU::write32( r1, 0 );
    }

    void erssector( u32 r0, u32 r1, u32 r2, u32 r3 ){
	u32 startPage = MMU::read32( r0+4 );
	u32 endPage = MMU::read32( r0+8 );
	u32 cclk = MMU::read32( r0+12 );
	if( verbose )
	    std::cout << "Erase sector" << std::endl;
	MMU::write32( r1, 0 );
    }

    void erspage( u32 r0, u32 r1, u32 r2, u32 r3 ){
	u32 startPage = MMU::read32( r0+4 );
	u32 endPage = MMU::read32( r0+8 );
	u32 cclk = MMU::read32( r0+12 );
	if( verbose )
	    std::cout << "Erase page"
		  << startPage
		  << " - "
		  << endPage
		  << " CCLK:" << cclk
		  << std::endl;
	
	MMU::write32( r1, 0 );
    }

    void repid( u32 r0, u32 r1, u32 r2, u32 r3 ){
	if( verbose )
	    std::cout << "Read Part ID" << std::endl;
    }
    
    void (*command[])(u32,u32,u32,u32) = {
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	prewrrite,wrisector,erssector,stub,repid,stub,stub,stub,stub,erspage,
	stub,
	&writeEEPROM, // 61
	&readEEPROM   // 62
    };

    void stub(u32 id,u32,u32,u32){
	if( verbose )
	    std::cout << "IAP STUB " << id << std::endl;
    }
    
}
