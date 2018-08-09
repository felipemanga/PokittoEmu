#include <exception>
#include <string>
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>

#include "cpu.hpp"
#include "timers.hpp"
#include "gpio.hpp"
#include "screen.hpp"
#include "initerror.hpp"
#include "state.hpp"
#include "gdb.hpp"

bool hasQuit = false;

class SDL
{
    SDL_Window * m_window;
    SDL_Renderer * m_renderer;
    SDL_Surface *vscreen;
    SDL_Surface *screen;
public:
    SDL( Uint32 flags = 0 );
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
}

SDL::~SDL()
{
    if( vscreen ){
	SDL_UnlockSurface(vscreen);
	SDL_FreeSurface(vscreen);
    }
    SDL_DestroyWindow( m_window );
    SDL_DestroyRenderer( m_renderer );
    SDL_Quit();
}

void SDL::draw()
{    
    SDL_UnlockSurface( vscreen );
    SDL_BlitScaled( vscreen, nullptr, screen, nullptr );
    SDL_UpdateWindowSurface(m_window);
    SDL_LockSurface(vscreen);
}

bool loadBin( const std::string &fileName ){
    std::ifstream inp( fileName, std::ios::binary );
    if( !inp.good() ) return false;
    inp.read( (char *) MMU::flash, sizeof(MMU::flash) );
    return true;
}

EmuState emustate = EmuState::RUNNING;

int main( int argc, char * argv[] ){

    if( !loadBin( argc > 1 ? argv[1] : "file.bin" ) ){
        std::cerr << "Error: Could not load file." << std::endl;
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

	while( !hasQuit ){
	    SDL_Event e;

	    GDB::update();
	    while (SDL_PollEvent(&e)) {
		
		if( e.type == SDL_QUIT ) 
		    return 0;

		if( emustate == EmuState::STOPPED ) 
		    continue;

		if( e.type == SDL_KEYDOWN || e.type == SDL_KEYUP ){
		    u32 btnState = e.type == SDL_KEYDOWN;
		    switch( e.key.keysym.sym ){
		    case SDLK_ESCAPE: return 0;
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
		for( u32 i=0; i<500000; i+=1 ){
		    CPU::cpuNextEvent += 5;
		    CPU::thumbExecute();
		    TIMERS::update();
		}
		break;

	    case EmuState::STEP:
		GPIO::update();
		CPU::cpuNextEvent++;
		CPU::thumbExecute();
		TIMERS::update();
		GDB::interrupt();
		break;
		
	    }

	    SCREEN::LCD[ 1*220+1 ] = CPU::cpuTotalTicks;
	    if( SCREEN::dirty ){
		sdl.draw();
		SCREEN::dirty = false;
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
