#include <exception>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <chrono>

#include "cpu.hpp"
#include "sys.hpp"
#include "timers.hpp"
#include "gpio.hpp"
#include "screen.hpp"
#include "spi.hpp"
#include "sd.hpp"
#include "sct.hpp"
#include "initerror.hpp"
#include "state.hpp"

#ifndef __EMSCRIPTEN__
#include "verify.hpp"
#include "prof.hpp"
#else
#include <emscripten/emscripten.h>
#endif
#include "gdb.hpp"

#include "./sdl.hpp"

volatile bool hasQuit = false;
std::string srcPath = "file.bin";
std::string imgPath;

bool verifier = false,
    autorec = false;
u32 verbose;

u16 debuggerPort = 0,
    profiler = 0;

SDL *sdl;

uint8_t rgba[220*176*4];

bool loadBin( const std::string &fileName );

void parseArgs( int argc, char *argv[] ){
    for( u32 i=1; i<argc; ++i ){
	const std::string arg = argv[i];

	if( arg.empty() ) continue;

	if( arg[0] == '-' && arg.size() == 2 ){
	    switch( arg[1] ){
	    case 'r':
		autorec = true;
		break;

	    case 'V':
		verbose++;
		break;

	    case 'w':
		if( i+1 >= argc ) MMU::ignoreBadWrites = 1;
		else MMU::ignoreBadWrites = atoi( argv[++i] );
		break;

	    case 'W':
		MMU::ignoreBadWrites = ~0;
		break;

	    case 'e':
		if( i+1 >= argc )
		    std::cout << "-e switch should be followed by entry point address." << std::endl;
		else
		    SYS::vtorReset = strtoul( argv[++i], NULL, 16 );
		break;

	    case 's':
		if( i+1>=argc || !(sdl->screenshot = atoi( argv[++i] )) )
		    std::cout << "-s switch should be followed by frames to skip before taking a screenshot." << std::endl;
		break;

	    case 'g':
		debuggerPort = 1234;
		break;

	    case 'G':		
		if( i+1>=argc || !(debuggerPort = atoi( argv[++i] )) )
		    std::cout << "-G switch should be followed by port number." << std::endl;

		break;

	    case 'I':
		if( i+1>=argc )
		    std::cout << "-I should be followed by Fat32 SD image." << std::endl;
		else
		    imgPath = argv[++i];

		break;

	    case 'p':
		profiler = 1;
		break;
		
	    case 'P': // Try to guess hot functions instead of instructions. Might guess wrong.
		profiler = 2;
		break;

	    case 'v':
		#ifdef __linux__
		verifier = true;
		#else
		std::cout << "Verifier only available on Linux." << std::endl;
		#endif
		break;

	    default:
		std::cout << "Unrecognized switch \"" << arg << "\"." << std::endl;
		break;
	    }
	}else{
	    srcPath = argv[i];
	}

    }
}

volatile EmuState emustate = EmuState::RUNNING;

extern "C" void reset(){
    emustate = EmuState::STOPPED;
    loadBin( srcPath );
    CPU::reset();
    emustate = EmuState::RUNNING;
}

extern "C" void takeScreenshot(){
    IMG_SavePNG( sdl->vscreen, "screenshot.png" );    
}

void loop( void *_sdl ){
    SDL &sdl = *(SDL *)_sdl;

    sdl.emitEvents();
    
#ifndef __EMSCRIPTEN__
    GDB::update();
    VERIFY::update();
#endif

    switch( emustate ){
    case EmuState::JUST_STOPPED:
	break;
    case EmuState::STOPPED:
	break;

    case EmuState::RUNNING:
	
	GPIO::update();
	
	{
	    auto start = std::chrono::high_resolution_clock::now();
	    u32 max = SYS::SYSPLLCTRL == 0x25 ? 0x140000 : 0xc0000;
            // (45 + (SYS::SYSPLLCTRL-0x23)*15)*1024*1024/30;// 150000*5;
            
	    for( u32 opcount=0; opcount<max && emustate == EmuState::RUNNING; ){
		u32 tti = max-opcount;
		tti = std::min( tti, SYS::update() );
		tti = std::min( tti, TIMERS::update() );
		CPU::cpuNextEvent = CPU::cpuTotalTicks + tti;
		CPU::thumbExecute();
		opcount += tti;
	    }
	    
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
	    
	    while( (std::chrono::high_resolution_clock::now() - start) < 16ms )
		std::this_thread::sleep_for( 1ms );
	    
#endif
	    
	}
	    
	break;

    case EmuState::STEP:
		
#ifndef __EMSCRIPTEN__
	GPIO::update();
	CPU::cpuNextEvent = CPU::cpuTotalTicks + 1;
	CPU::thumbExecute();
	TIMERS::update();
	GDB::interrupt();
#endif
		
	break;
		
    }

    // SCREEN::LCD[ 1*220+1 ] = CPU::cpuTotalTicks;

#ifndef __EMSCRIPTEN__
	    
    if( emustate != EmuState::RUNNING ){
	std::this_thread::sleep_for( 50ms );
    }
    
#endif
    
}

void drawLoop( void *_sdl ){
    ((SDL*)_sdl)->draw();
    ((SDL*)_sdl)->checkEvents();
}

void drawAndCpuLoop( void *_sdl ){
    ((SDL*)_sdl)->draw();
    ((SDL*)_sdl)->checkEvents();
    loop( _sdl );
}

std::thread cputhread;

int main( int argc, char * argv[] ){

    CPU::cpuNextEvent = 0;

    try{

        SDL sdl( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK );
	::sdl = &sdl;

        parseArgs( argc, argv );

        srcPath = std::regex_replace( srcPath, std::regex(R"(\.elf$)", std::regex_constants::icase), ".bin" );
        if( !loadBin( srcPath ) ){
            std::cerr << "Error: Could not load file. [" << srcPath << "]" << std::endl;
            return 1;
        }

	#ifndef __EMSCRIPTEN__
	if( debuggerPort && !GDB::init( debuggerPort ) )
	    throw InitError("\\o/");
	#endif

        MMU::init();
	if( imgPath.empty() ){
	}else if( !SD::init( imgPath ) ){
	    std::cerr << "Error: Could not open image file. ["
		      << imgPath
		      << "]"
		      << std::endl;
	    return 2;
	}else{
	    SPI::spi0Out( SD::write );
	}	
	
        CPU::init();
        CPU::reset();

	#ifndef __EMSCRIPTEN__
	if( profiler )
	    PROF::init( profiler == 2 );

	if( verifier )
	    VERIFY::init();
	#endif

	if( autorec )
	    sdl.toggleRecording();

	#ifndef __EMSCRIPTEN__
	
	cputhread = std::thread( []( void *sdl ){
		while( !hasQuit )
		    loop(sdl);
	    }, (void *) &sdl );
	
	while( !hasQuit ){
	    drawLoop( &sdl );
	}

	cputhread.join();
	
	#elif !defined(__EMSCRIPTEN_PTHREADS__)

	emscripten_set_main_loop_arg( drawAndCpuLoop, &sdl, -1, 1 );	
	
        #else
	
	cputhread = std::thread( []( void *sdl ){
		while(true){ loop(sdl); }
	    }, (void *) &sdl );
	
	emscripten_set_main_loop_arg( drawLoop, &sdl, -1, 1 );
	
        #endif


        return 0;
    }
    catch ( const InitError & err )
    {
        std::cerr << "Error while initializing SDL:  "
                  << err.what()
                  << std::endl;
    }

    return 1;
}
