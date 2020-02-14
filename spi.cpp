#include "spi.hpp"
#include "cpu.hpp"

#include <vector>

namespace SPI {

    struct SPI {
	u32 id;
	u32 CR0, CR1, DR, SR, CPSR, IMSC, RIS, MIS, ICR, DMACR;

	std::vector< Listener > listeners;
	std::vector< u8 > inBuffer;
	
	SPI( u32 id ) : id(id){
	    init();
	}
	
	void init(){
	    CR0=0;
	    CR1=0;
	    DR=0;
	    SR=3;
	    CPSR=0;
	    IMSC=0x8;
	    RIS=0;
	    MIS=0;
	    ICR=0;
	    DMACR=0;
	}

	u32 dataSize,
	    format, // 0-SPI, 1-TI, 2-Microwire
	    loopBack,
	    enabled,
	    slave,
	    slaveOutputDisabled;
	

    } SPI0(0), SPI1(1);

    void init(){
	SPI0.init();
	SPI1.init();
    }

    void spi0In( u32 v, bool clear ){
        if(clear) SPI0.inBuffer.clear();
	SPI0.inBuffer.insert( SPI0.inBuffer.begin(), v );
	SPI0.SR = (3) | 4;
    }

    void spi1In( u32 v ){
	SPI1.inBuffer.insert( SPI1.inBuffer.begin(), v );
	SPI1.SR = (3) | 4;
    }

    void spi0Out( Listener l ){
	SPI0.listeners.push_back(l);
    }

    void spi1Out( Listener l ){
	SPI1.listeners.push_back(l);
    }

    template <SPI &spi>
    u32 writeCR0( u32 v, u32 ov, u32 addr ){
	spi.dataSize = v & 0xF;
	spi.format = (v>>4) & 0x3;
	return v;
    }

    template <SPI &spi>
    u32 writeCR1( u32 v, u32 ov, u32 addr ){
	spi.loopBack = v & 1;
	spi.enabled = (v>>1) & 1;
	spi.slave = (v>>2) & 1;
	spi.slaveOutputDisabled = (v>>3) & 1;
	return v;
    }

    template <SPI &spi>
    u32 writeDR( u32 v, u32 ov, u32 addr ){
	for( auto listener : spi.listeners )
	    listener( v );
	spi.SR = (3) | (spi.inBuffer.empty() ? 0 : 4);
	return v;
    }

    template <SPI &spi>
    u32 readDR( u32 v, u32 addr ){
	if( !spi.inBuffer.empty() ){
	    v = spi.inBuffer.back();
	    spi.inBuffer.pop_back();
	    spi.SR = (3) | (spi.inBuffer.empty() ? 0 : 4);
	}
	return v;
    }

    template <SPI &spi>
    u32 readSR( u32 v, u32 addr ){
	return v;
    }

    MMU::Register spi0Map[] = {
	MMUREGW(SPI0.CR0, writeCR0<SPI0> ),
	MMUREGW(SPI0.CR1, writeCR1<SPI0> ),
	MMUREGRW( SPI0.DR, readDR<SPI0>, writeDR<SPI0> ),
	MMUREGRO( SPI0.SR, readSR<SPI0> ),
	MMUREG( SPI0.CPSR ),
	MMUREG( SPI0.IMSC ),
	MMUREG( SPI0.RIS ),
	MMUREG( SPI0.MIS ),
	MMUREG( SPI0.ICR ),
	MMUREG( SPI0.DMACR )
    };

    MMU::Register spi1Map[] = {
	MMUREGW(SPI1.CR0, writeCR0<SPI1> ),
	MMUREGW(SPI1.CR1, writeCR1<SPI1> ),
	MMUREGRW( SPI1.DR, readDR<SPI1>, writeDR<SPI1> ),
	MMUREGRO( SPI1.SR, readSR<SPI1> ),
	MMUREG( SPI1.CPSR ),
	MMUREG( SPI1.IMSC ),
	MMUREG( SPI1.RIS ),
	MMUREG( SPI1.MIS ),
	MMUREG( SPI1.ICR ),
	MMUREG( SPI1.DMACR )
    };

    MMU::Layout spi0Layout = {
	0x40040000,
	sizeof(spi0Map) / sizeof(spi0Map[0]),
	spi0Map
    };

    MMU::Layout spi1Layout = {
	0x40058000,
	sizeof(spi1Map) / sizeof(spi1Map[0]),
	spi1Map
    };

}
