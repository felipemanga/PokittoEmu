#include <iostream>
#include "types.hpp"
#include "iap.hpp"
#include "mmu.hpp"


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
    
    void (*command[])(u32,u32,u32,u32) = {
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,stub,stub,stub,stub,stub,stub,stub,stub,stub,
	stub,
	&writeEEPROM, // 61
	&readEEPROM   // 62
    };

    void stub(u32 id,u32,u32,u32){
	std::cout << "IAP STUB " << id << std::endl;
    }
    
}
