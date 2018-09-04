#include <exception>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

#include <SDL2/SDL.h>
#include "gif.h"

#include "cpu.hpp"
#include "timers.hpp"
#include "gpio.hpp"
#include "screen.hpp"
#include "initerror.hpp"
#include "state.hpp"
#include "gdb.hpp"
#include "prof.hpp"

volatile bool hasQuit = false;
std::string srcPath;

class SDL
{
    SDL_Window * m_window;
    SDL_Renderer * m_renderer;
    SDL_Surface *vscreen;
    SDL_Surface *screen;
    std::thread worker;

    u8 *screenpixels;
    u32 gifNum = 0;
    bool recording = false;
    std::mutex gifmut;
    GifWriter gif;

public:
    SDL( Uint32 flags = 0 );
    void toggleRecording();
    virtual ~SDL();
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
    
    worker = std::thread( &SDL::draw, this );
}

SDL::~SDL()
{
    
    if( recording )
	toggleRecording();

    hasQuit = true;
    worker.join();
    
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
    std::lock_guard<std::mutex> gml(gifmut);
    
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

void SDL::draw()
{
    uint8_t rgba[220*176*4];
    
    while( !hasQuit ){
	std::this_thread::sleep_for( std::chrono::milliseconds(10) );
	if( !SCREEN::dirty ) continue;

	SDL_UnlockSurface( vscreen );
	SDL_BlitScaled( vscreen, nullptr, screen, nullptr );
	
	{
	    std::lock_guard<std::mutex> gml(gifmut);

	    if( recording ){

		uint8_t *rgbap = rgba;
		for( u32 i=0; i<220*176; ++i ){
		    u16 c = ((u16 *)SCREEN::LCD)[i];
		    *rgbap++ = float(c >> 11) / 0x1F * 255.0f;
		    *rgbap++ = float((c >> 5) & 0x3F) / 0x3F * 255.0f;
		    *rgbap++ = float(c & 0x1F) / 0x1F * 255.0f;
		    *rgbap++ = 255;
		}
		
		GifWriteFrame( &gif, (u8*) rgba, 220, 176, 2, 8 );

		for( u32 y=0; y<15; ++y ){
		    for( u32 x=0; x<15; ++x ){
			screenpixels[ (((y+3)*440+x+3)<<2)+2 ] = ~CPU::cpuTotalTicks;
		    }
		}
		
	    }
	    
	}

	SDL_UpdateWindowSurface(m_window);
	SDL_LockSurface(vscreen);
	SCREEN::dirty = false;
    }
}

bool loadBin( const std::string &fileName ){
    std::ifstream inp( fileName, std::ios::binary );
    if( !inp.good() ) return false;
    inp.read( (char *) MMU::flash, sizeof(MMU::flash) );
    return true;
}

EmuState emustate = EmuState::RUNNING;

int main( int argc, char * argv[] ){
    if( argc > 1 ) srcPath = argv[1];
    else srcPath = "file.bin";

    srcPath = std::regex_replace( srcPath, std::regex(R"(\.elf$)", std::regex_constants::icase), ".bin" );

    if( !loadBin( srcPath ) ){
        std::cerr << "Error: Could not load file. [" << srcPath << "]" << std::endl;
        return 1;
    }

    CPU::cpuNextEvent = 0;

    try{

        SDL sdl( SDL_INIT_VIDEO | SDL_INIT_TIMER );

	if( argc > 2 && argv[2] == std::string("-g") ){

	    u32 port = 0;
	    if( argc > 3 )
		port = atoi( argv[3] );

	    if( !GDB::init( port ) )
		throw InitError();
	    
	}

        MMU::init();
        CPU::init();
        CPU::reset();

	if( argc > 2 && (argv[2] == std::string("-p") || argv[2] == std::string("-P")) ){
	    PROF::init( argv[2] == std::string("-P") );
	}

	while( !hasQuit ){
	    SDL_Event e;

	    GDB::update();
	    while (SDL_PollEvent(&e)) {
		
		if( e.type == SDL_QUIT ){
		    hasQuit = true;
		    return 0;
		}

		if( emustate == EmuState::STOPPED ) 
		    continue;

		if( e.type == SDL_KEYDOWN || e.type == SDL_KEYUP ){
		    u32 btnState = e.type == SDL_KEYDOWN;
		    switch( e.key.keysym.sym ){
		    case SDLK_ESCAPE: return 0;
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
	    case EmuState::STOPPED:
		break;

	    case EmuState::RUNNING:
		GPIO::update();
		for( u32 opcount=0; opcount<500000 && emustate == EmuState::RUNNING; ++opcount ){
		    CPU::cpuNextEvent = CPU::cpuTotalTicks + 5;
		    CPU::thumbExecute();
		    TIMERS::update();
		}
		break;

	    case EmuState::STEP:
		GPIO::update();
		CPU::cpuNextEvent = CPU::cpuTotalTicks + 1;
		CPU::thumbExecute();
		TIMERS::update();
		GDB::interrupt();
		break;
		
	    }

	    SCREEN::LCD[ 1*220+1 ] = CPU::cpuTotalTicks;
	    if( emustate != EmuState::RUNNING ){
		std::this_thread::sleep_for( std::chrono::milliseconds(50) );
	    }else{
		std::this_thread::sleep_for( std::chrono::milliseconds(20) );
	    }
	}

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
