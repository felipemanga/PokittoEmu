#include <exception>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include "gif.h"

#include "cpu.hpp"
#include "timers.hpp"
#include "gpio.hpp"
#include "screen.hpp"
#include "initerror.hpp"
#include "state.hpp"

#include <chrono>
#include <thread>
#include <mutex>

#ifndef __EMSCRIPTEN__
#include "verify.hpp"
#include "prof.hpp"
#else
#include <emscripten/emscripten.h>
#endif
#include "gdb.hpp"

using namespace std::chrono_literals;

volatile bool hasQuit = false;
std::string srcPath = "file.bin";

class SDL
{
    SDL_Window * m_window;
    SDL_Renderer * m_renderer;
    SDL_Surface *vscreen;
    SDL_Surface *screen;

    u8 *screenpixels;
    u32 gifNum = 0;
    u32 delay = 2;
    bool recording = false;
#ifndef __EMSCRIPTEN__
    std::mutex gifmut;
#endif
    GifWriter gif;

public:
    SDL( Uint32 flags = 0 );
    void toggleRecording();
    virtual ~SDL();
    void thread();
    void draw();
};

SDL::SDL( Uint32 flags )
{
    if ( SDL_Init( flags ) != 0 )
        throw InitError();

    m_window = SDL_CreateWindow("PokittoEmu",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             220*2, 176*2,
                             0);
    if( !m_window )
	throw InitError();

    m_renderer = SDL_CreateRenderer(
	m_window,
	-1,
	SDL_RENDERER_SOFTWARE// | SDL_RENDERER_PRESENTVSYNC
	);

    if( !m_renderer )
	throw InitError();

    recording = false;

    // Clear the window with a black background
    SDL_SetRenderDrawColor( m_renderer, 0, 0, 0, 255 );
    SDL_RenderClear( m_renderer );
    screen = SDL_GetWindowSurface( m_window );
    vscreen = SDL_CreateRGBSurface(
	0, // flags
	220, // w
	176, // h
	16, // depth
	0,
	0,
	0,
	0
	);

    if( !vscreen )
	throw InitError();

    SDL_LockSurface(vscreen);
    SCREEN::LCD = (u16 *) vscreen->pixels;

    SDL_LockSurface(screen);
    screenpixels = (u8 *) screen->pixels;
    SDL_UnlockSurface( screen );

}

SDL::~SDL()
{

    hasQuit = true;
    
    if( recording )
	toggleRecording();
    
    if( vscreen ){
	SDL_UnlockSurface(vscreen);
	SDL_FreeSurface(vscreen);
    }
    SDL_DestroyWindow( m_window );
    SDL_DestroyRenderer( m_renderer );
    SDL_Quit();
    
}

void SDL::toggleRecording(){
    recording = !recording;
#ifndef __EMSCRIPTEN__
    std::lock_guard<std::mutex> gml(gifmut);
#endif
    
    if( recording ){
	gifNum++;
	std::string name = srcPath;
	name += ".";
	name += std::to_string(gifNum);
	name += ".gif";
	GifBegin( &gif, name.c_str(), 220, 176, 2 );
    }else{
	GifEnd( &gif );
    }
    
}

void SDL::thread()
{
    while( !hasQuit ){
	std::this_thread::sleep_for( std::chrono::milliseconds(10) );
	draw();
    }
}

uint8_t rgba[220*176*4];

void SDL::draw(){
    if( !SCREEN::dirty ){
	delay+=4;
	return;
    }

    SDL_UnlockSurface( vscreen );
    SDL_BlitScaled( vscreen, nullptr, screen, nullptr );
	
    {
#ifndef __EMSCRIPTEN__
	std::lock_guard<std::mutex> gml(gifmut);
#endif
	if( recording ){

	    uint8_t *rgbap = rgba;
	    for( u32 i=0; i<220*176; ++i ){
		u16 c = ((u16 *)SCREEN::LCD)[i];
		*rgbap++ = float(c >> 11) / 0x1F * 255.0f;
		*rgbap++ = float((c >> 5) & 0x3F) / 0x3F * 255.0f;
		*rgbap++ = float(c & 0x1F) / 0x1F * 255.0f;
		*rgbap++ = 255;
	    }
		
	    GifWriteFrame( &gif, (u8*) rgba, 220, 176, delay, 8 );

	    for( u32 y=0; y<15; ++y ){
		for( u32 x=0; x<15; ++x ){
		    screenpixels[ (((y+3)*440+x+3)<<2)+2 ] = ~CPU::cpuTotalTicks;
		}
	    }
		
	}
	    
    }
//    SDL_RenderPresent( m_renderer );
    SDL_UpdateWindowSurface(m_window);
    SDL_LockSurface(vscreen);
    SCREEN::dirty = false;
    delay = 4;

}

bool loadBin( const std::string &fileName ){
    std::ifstream inp( fileName, std::ios::binary );
    if( !inp.good() ) return false;
    inp.read( (char *) MMU::flash, sizeof(MMU::flash) );
    return true;
}

bool multithread = false,
    verifier = false,
    autorec = false;
u16 debuggerPort = 0,
    profiler = 0;

void parseArgs( int argc, char *argv[] ){
    for( u32 i=1; i<argc; ++i ){
	const std::string arg = argv[i];

	if( arg.empty() ) continue;

	if( arg[0] == '-' && arg.size() == 2 ){
	    switch( arg[1] ){
	    case 'r':
		autorec = true;
		break;

	    case 't':
		multithread = true;
		break;

	    case 'g':
		debuggerPort = 1234;
		break;

	    case 'G':		
		if( i+1<argc && !(debuggerPort = atoi( argv[++i] )) )
		    std::cout << "-G switch should be followed by port number." << std::endl;

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

void loop( void *_sdl ){
    SDL &sdl = *(SDL *)_sdl;

    SDL_Event e;
#ifndef __EMSCRIPTEN__
    GDB::update();
    VERIFY::update();
#endif

    while (SDL_PollEvent(&e)) {
		
	if( e.type == SDL_QUIT ){
	    hasQuit = true;
	    return;
	}

	if( e.type == SDL_KEYUP ){
	    switch( e.key.keysym.sym ){
	    case SDLK_ESCAPE:
		hasQuit = true;
		return;
		
	    case SDLK_F5:
		reset();
		break;
		
	    case SDLK_F7:
		if( emustate == EmuState::RUNNING )
		    emustate = EmuState::STOPPED;
		else
		    emustate = EmuState::RUNNING;
		break;
		
	    }
	}

	if( emustate == EmuState::STOPPED ) 
	    continue;

	if( e.type == SDL_KEYDOWN || e.type == SDL_KEYUP ){
	    u32 btnState = e.type == SDL_KEYDOWN;
	    switch( e.key.keysym.sym ){		
	    case SDLK_F3:
		if(e.type == SDL_KEYDOWN)
		    sdl.toggleRecording();
		break;

	    case SDLK_F9: GDB::interrupt(); break;
	    case SDLK_UP: GPIO::input(1,13,btnState); break;
	    case SDLK_DOWN: GPIO::input(1,3,btnState); break;
	    case SDLK_LEFT: GPIO::input(1,25,btnState); break;
	    case SDLK_RIGHT: GPIO::input(1,7,btnState); break;
	    case SDLK_a: GPIO::input(1,9,btnState); break;
	    case SDLK_s:
	    case SDLK_b: GPIO::input(1,4,btnState); break;
	    case SDLK_d:
	    case SDLK_c: GPIO::input(1,10,btnState); break;
	    }
	}

    }

    switch( emustate ){
    case EmuState::JUST_STOPPED:
	break;
    case EmuState::STOPPED:
	break;

    case EmuState::RUNNING:
	
	GPIO::update();
	
	{
	    auto start = std::chrono::high_resolution_clock::now();
	    for( u32 opcount=0; opcount<200000 && emustate == EmuState::RUNNING; ++opcount ){
		CPU::cpuNextEvent = CPU::cpuTotalTicks + 5;
		CPU::thumbExecute();
		TIMERS::update();
	    }
	    
	    while( (std::chrono::high_resolution_clock::now() - start) < 16ms )
		std::this_thread::sleep_for( 1ms );
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

    SCREEN::LCD[ 1*220+1 ] = CPU::cpuTotalTicks;

#ifndef __EMSCRIPTEN__
	    
    if( emustate != EmuState::RUNNING ){
	std::this_thread::sleep_for( 50ms );
    }
    
#endif
    
}

void drawLoop( void *_sdl ){
    ((SDL*)_sdl)->draw();
}

void drawAndCpuLoop( void *_sdl ){
    ((SDL*)_sdl)->draw();
    loop( _sdl );
}

std::thread cputhread;

int main( int argc, char * argv[] ){

    parseArgs( argc, argv );

    srcPath = std::regex_replace( srcPath, std::regex(R"(\.elf$)", std::regex_constants::icase), ".bin" );
    if( !loadBin( srcPath ) ){
        std::cerr << "Error: Could not load file. [" << srcPath << "]" << std::endl;
        return 1;
    }

    CPU::cpuNextEvent = 0;

    try{

        SDL sdl( SDL_INIT_VIDEO | SDL_INIT_TIMER );

	#ifndef __EMSCRIPTEN__
	if( debuggerPort && !GDB::init( debuggerPort ) )
	    throw InitError();
	#endif

        MMU::init();
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
	
	while( !hasQuit )
	    drawLoop( &sdl );

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
