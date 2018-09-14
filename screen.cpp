#include <iostream>
#include "types.hpp"
#include <SDL2/SDL.h>

extern bool verbose;

namespace SCREEN {

u16 *LCD;

#define HEIGHT 176
#define WIDTH  220

u32 colStart = 0;
u32 colEnd = HEIGHT-1;
u32 pageStart = 0;
u32 pageEnd = WIDTH-1;
u32 col = 0;
u32 page = 0;
u32 pageDelta = 1;
u32 colDelta = 1;
u32 dirty = true;
u32 V = false;
u32 vblank = false;


void LCDReset(){
    
    if( verbose )
	std::cout << "Screen reset" << std::endl;
    
    colStart = 0;
    colEnd = HEIGHT-1;
    pageStart = 0;
    pageEnd = WIDTH-1;
    col = 0;
    page = 0;
    pageDelta = 1;
    colDelta = 1;
    dirty = true;
    V = false;
}

u8 stubId;
void stub( u16 D ){
    if( verbose )
	std::cout << "ST7775 Stub "
		  << std::hex
		  << u32(stubId)
		  << std::endl;
}

void (*cmd)( u16 D ) = stub;


void cmd03( u16 D ){ // Entry Mode
    // console.log("Entry mode:", D.toString(16));
    V = D&0x08;
}

void cmd20( u16 D ){ // Horizontal DRAM Address Set
    col = D - colStart;
}

void cmd21( u16 D ){ // Vertical DRAM Address Set
    page = D - pageStart;
}

void cmd22( u16 D ){
    s32 cs = colStart;
    s32 ce = colEnd;
    s32 cd = ce - cs;
    s32 ps = pageStart;
    s32 pe = pageEnd;
    s32 pd = pe - ps;
    s32 x = cs + col;
    s32 y = ps + page;

    // std::cout << x << " " << y << " = " << std::hex << D << std::endl;
    
    if( !(x < 0 || x >= HEIGHT || y < 0 || y >= WIDTH) )
	LCD[ x*WIDTH+y ] = D;

    vblank = false;
    if( !V ){
        col++;
	if( col > cd ){
	    col = 0;
	    page++;
	    if( page > pd ){
	        vblank = true;
	        page = 0;
	    }
	}
    }else{
        page++;
	if( page > pd ){
	    page = 0;
	    col++;
	    if( col > cd ){
	        vblank = true;
	        col = 0;
	    }
	}
    }
    dirty = true;
    
}

void cmd36( u16 D ){ // Horizontal Address End
    colEnd = D;
}

void cmd37( u16 D ){
    colStart = D;
}

void cmd38( u16 D ){
    pageEnd = D;
}

void cmd39( u16 D ){
    pageStart = D;
}

void LCDWrite( u32 cd, u16 v ){
    // LCD[ 88*220 + 110 ] = v;
    if( cd )
	cmd( v );
    else{
	switch( v ){
	case 0x37: cmd = cmd37; break;
	case 0x36: cmd = cmd36; break;
	case 0x22: cmd = cmd22; break;
	case 0x21: cmd = cmd21; break;
	case 0x20: cmd = cmd20; break;
	case 0x03: cmd = cmd03; break;
	case 0x38: cmd = cmd38; break;
	case 0x39: cmd = cmd39; break;
	default: stubId = v; cmd=stub; break;
	}
    }
}

}
