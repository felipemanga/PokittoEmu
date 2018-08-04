#include <exception>
#include <string>
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>

#include "cpu.hpp"
#include "timers.hpp"
#include "screen.hpp"

class InitError : public std::exception
{
    std::string msg;
public:
    InitError() : exception(), msg( SDL_GetError() ){}
    InitError( const std::string & );
    virtual ~InitError() throw() {}
    virtual const char * what() const throw() {
        return msg.c_str();
    }
};

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
    LCD = (u16 *) vscreen->pixels;
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
    // SDL_RenderPresent( m_renderer );
    // SDL_RenderClear( m_renderer );
    
    SDL_LockSurface(vscreen);
}

bool loadBin( const std::string &fileName ){
    std::ifstream inp( fileName, std::ios::binary );
    if( !inp.good() ) return false;
    inp.read( (char *) MMU::flash, sizeof(MMU::flash) );
    return true;
}

int main( int argc, char * argv[] ){

    if( !loadBin( argc > 1 ? argv[1] : "file.bin" ) ){
        std::cerr << "Error: Could not load file." << std::endl;
        return 1;
    }

    CPU::cpuNextEvent = 0;

    try{

        SDL sdl( SDL_INIT_VIDEO | SDL_INIT_TIMER );
        MMU::init();
        CPU::init();

        CPU::reset();

	while( true ){
	    SDL_Event e;
	    while (SDL_PollEvent(&e)) {
		if( e.type == SDL_QUIT ) return 0;
	    }

	    for( u32 i=0; i<50000; i+=10 ){
		for( u32 j=0; j<10; j++ ){
		    CPU::cpuNextEvent += 10;
		    CPU::thumbExecute();
		}
		TIMERS::update();
	    }

	    LCD[ 1*220+1 ] = CPU::cpuTotalTicks;

	    sdl.draw();
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
