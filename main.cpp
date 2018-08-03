#include <exception>
#include <string>
#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>

#include "cpu.hpp"

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
public:
    SDL( Uint32 flags = 0 );
    virtual ~SDL();
    void draw();
};

SDL::SDL( Uint32 flags )
{
    if ( SDL_Init( flags ) != 0 )
        throw InitError();

    if ( SDL_CreateWindowAndRenderer( 640, 480, SDL_WINDOW_SHOWN,
                                      &m_window, &m_renderer ) != 0 )
        throw InitError();
}

SDL::~SDL()
{
    SDL_DestroyWindow( m_window );
    SDL_DestroyRenderer( m_renderer );
    SDL_Quit();
}

void SDL::draw()
{
    // Clear the window with a black background
    SDL_SetRenderDrawColor( m_renderer, 0, 0, 0, 255 );
    SDL_RenderClear( m_renderer );

    // Show the window
    SDL_RenderPresent( m_renderer );

    int rgb[] = { 203, 203, 203, // Gray
                  254, 254,  31, // Yellow
                    0, 255, 255, // Cyan
                    0, 254,  30, // Green
                  255,  16, 253, // Magenta
                  253,   3,   2, // Red
                   18,  14, 252, // Blue
                    0,   0,   0  // Black
                };

    SDL_Rect colorBar;
    colorBar.x = 0; colorBar.y = 0; colorBar.w = 90; colorBar.h = 480;

    // Render a new color bar every 0.5 seconds
    for ( int i = 0; i != sizeof rgb / sizeof *rgb; i += 3, colorBar.x += 90 )
    {
        SDL_SetRenderDrawColor( m_renderer, rgb[i], rgb[i + 1], rgb[i + 2], 255 );
        SDL_RenderFillRect( m_renderer, &colorBar );
        SDL_RenderPresent( m_renderer );
        SDL_Delay( 500 );
    }
}

bool loadBin( const std::string &fileName ){
    std::ifstream inp( fileName, std::ios::binary );
    if( !inp.good() ) return false;
    inp.read( (char *) MMU::flash, sizeof(MMU::flash) );
    return true;
}

int main( int argc, char * argv[] ){

    if( !loadBin( argc > 1 ? argv[1] : "firmware.bin" ) ){
        std::cerr << "Error: Could not load file." << std::endl;
        return 1;
    }

    CPU::cpuNextEvent = 10000;

    try{

        // SDL sdl( SDL_INIT_VIDEO | SDL_INIT_TIMER );
        // sdl.draw();
        MMU::init();
        CPU::init();
        // std::cout << std::hex << MMU::read32(0) << std::endl;
        CPU::reset();
        CPU::thumbExecute();
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
