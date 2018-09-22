#include "types.hpp"
#include "usart.hpp"
#include "cpu.hpp"
#include <iostream>
#include <vector>

namespace USART {

    struct USART0t {

	u32 RBRTHRDLL,
	    DLMIER, IIRFCR,
	    LCR, MCR,
	    LSR, MSR, SCR,
	    ACR, ICR, FDR,
	    OSR, TER, HDEN,
	    SCICTRL, RS485CTRL, RS485ADDRMATCH,
	    RS485DLY, SYNCCTRL;

	std::vector< Listener > listeners;
	std::vector< u8 > inBuffer;
	
	USART0t(){
	    init();
	}
	
	void init(){
	    RBRTHRDLL=1;
	    DLMIER=0; IIRFCR=0;
	    LCR=0; MCR=0;
	    LSR=0x60; MSR=0; SCR=0;
	    ACR=0; ICR=0; FDR=0x10;
	    OSR=0xF0; TER=0x80; HDEN=0;
	    SCICTRL=0; RS485CTRL=0; RS485ADDRMATCH=0;
	    RS485DLY=0; SYNCCTRL=0;
	}
	
    } USART0;

    void init(){
	USART0.init();
    }

    void usart0In( u32 v ){
	USART0.inBuffer.insert( USART0.inBuffer.begin(), v );
    }

    void usart0Out( Listener l ){
	USART0.listeners.push_back(l);
    }

    u32 readUSART0( u32 v, u32 addr ){
	return v;
    }
    u32 writeUSART0( u32 v, u32 ov, u32 addr ){
	if( USART0.LCR == 3 ){
	    if( char(v) == 10 ) std::cout << std::endl;
	    else std::cout << char(v);
	}
	return v;
    }

    MMU::Register usart0Map[] = {
	MMUREGRW( USART0.RBRTHRDLL, readUSART0, writeUSART0 ),
	
	MMUREG(USART0.DLMIER),
	MMUREG(USART0.IIRFCR),
	MMUREG(USART0.LCR),
	MMUREG(USART0.MCR),
	
	MMUREG(USART0.LSR),
	MMUREG(USART0.MSR),
	MMUREG(USART0.SCR),
	
	MMUREG(USART0.ACR),
	MMUREG(USART0.ICR),
	MMUREG(USART0.FDR),
	
	MMUREG(USART0.OSR),
	MMUREG(USART0.TER),
	MMUREG(USART0.HDEN),
	
	MMUREG(USART0.SCICTRL),
	MMUREG(USART0.RS485CTRL),
	MMUREG(USART0.RS485ADDRMATCH),
	
	MMUREG(USART0.RS485DLY),
	MMUREG(USART0.SYNCCTRL)	
    };

    MMU::Layout usart0Layout = {
	0x40008000,
	sizeof(usart0Map) / sizeof(usart0Map[0]),
	usart0Map
    };

}
