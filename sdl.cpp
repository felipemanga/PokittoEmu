#include "sdl.hpp"

#include "./cpu.hpp"
#include "./gdb.hpp"
#include "./gpio.hpp"
#include "./screen.hpp"
#include "./state.hpp"
#include "./sd.hpp"
#include "./adc.hpp"
#include "gif.h"

extern volatile EmuState emustate;
extern volatile bool hasQuit;
extern std::string srcPath;
extern "C" void reset();

GifWriter gif;

SDL::SDL( Uint32 flags )
{
    if ( SDL_Init( flags ) != 0 )
        throw InitError(SDL_GetError());

    canTakeScreenshot = (IMG_Init( IMG_INIT_PNG ) & IMG_INIT_PNG) == IMG_INIT_PNG;

    m_window = SDL_CreateWindow("PokittoEmu",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             220*2, 176*2,
                             0);
    if( !m_window )
	throw InitError(SDL_GetError());

    m_renderer = SDL_CreateRenderer(
	m_window,
	-1,
	SDL_RENDERER_SOFTWARE// | SDL_RENDERER_PRESENTVSYNC
	);

    if( !m_renderer )
	throw InitError(SDL_GetError());

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
	throw InitError(SDL_GetError());

    SDL_LockSurface(vscreen);
    SCREEN::LCD = (u16 *) vscreen->pixels;

    SDL_LockSurface(screen);
    screenpixels = (u8 *) screen->pixels;
    SDL_UnlockSurface( screen );

    SDL_JoystickEventState(SDL_ENABLE);
    u32 numJoysticks = SDL_NumJoysticks();
    for( u32 i=0; i<numJoysticks; ++i ){
        joysticks[i] = SDL_JoystickOpen(i);
    }
}

SDL::~SDL()
{
    for( auto js : joysticks )
        SDL_JoystickClose(js.second);

    hasQuit = true;
    
    if( recording )
	toggleRecording();
    
    if( vscreen ){
	SDL_UnlockSurface(vscreen);
	SDL_FreeSurface(vscreen);
    }

    IMG_Quit();

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
	delay = 10;
    }else{
	GifEnd( &gif );
    }
    
}

void SDL::savePNG(){
    gifNum++;
    std::string name = srcPath;
    name += ".";
    name += std::to_string(gifNum);
    name += ".png";

    IMG_SavePNG( vscreen, name.c_str() );
}

void SDL::draw(){
    if( !SCREEN::dirty ){
	delay+=2;
	#ifndef __EMSCRIPTEN__
	std::this_thread::sleep_for( 10ms );
	#endif
	return;
    }
    SCREEN::dirty = false;

    SDL_UnlockSurface( vscreen );
    SDL_BlitScaled( vscreen, nullptr, screen, nullptr );
    SDL_LockSurface(vscreen);

    if( screenshot ){
	screenshot--;
	if( !screenshot )
	    savePNG();
    }
	
    {
#ifndef __EMSCRIPTEN__
	std::lock_guard<std::mutex> gml(gifmut);
#endif
	if( recording && delay > 4 ){

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
		
	    delay = 2;
	    
	}
	    
    }

    SDL_UpdateWindowSurface(m_window);
//    SDL_RenderPresent( m_renderer );

}

void SDL::emitEvents(){
    std::lock_guard<std::mutex> lock(eventmut);

    for( EventHandler ev : eventHandlers ){
        ev();
    }

    eventHandlers.clear();
}


void SDL::checkEvents(){
    SDL_Event e;

    if( !joysticks.empty() ){
        ADC::DAT8 = (int32_t(SDL_JoystickGetAxis(joysticks.begin()->second, 4))>>1)+0x8000;
        ADC::DAT9 = (int32_t(-SDL_JoystickGetAxis(joysticks.begin()->second, 3))>>1)+0x8000;
    }else{
        ADC::DAT8 = 0x8000;
        ADC::DAT9 = 0x8000;
    }
    
    while (SDL_PollEvent(&e)) {
	std::lock_guard<std::mutex> lock(eventmut);
		
	if( e.type == SDL_QUIT ){
	    hasQuit = true;
	    return;
	}

        if( e.type == SDL_JOYBUTTONUP || e.type == SDL_JOYBUTTONDOWN ){
            u32 btnState = e.type == SDL_JOYBUTTONDOWN;
            switch( e.jbutton.button ){
            case 0: // a
		eventHandlers.push_back( [=](){
			GPIO::input(1,9,btnState); 
		    } );
                break;
            case 1: // b
		eventHandlers.push_back( [=](){
			GPIO::input(1,4,btnState); 
		    } );
                break;
            case 2: // c
		eventHandlers.push_back( [=](){
			GPIO::input(1,10,btnState); 
		    } );
                break;
            case 3: // flash
		eventHandlers.push_back( [=](){
			GPIO::input(0,1,btnState); 
		    } );
                break;
            }
        }else if( e.type == SDL_JOYAXISMOTION ){
            static int px = 0, py = 0;
            int x = 0, y = 0;
            const int deadzone = 1000;

            if( e.jaxis.axis == 0) 
            {
                if( e.jaxis.value < -deadzone ) x = -1;
                else if( e.jaxis.value > deadzone ) x = 1;
            }

            if( e.jaxis.axis == 1) 
            {
                if( e.jaxis.value < -deadzone ) y = -1;
                else if( e.jaxis.value > deadzone ) y = 1;                
            }

            if( x != px ){
                
                if( px < 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,25,false); 
                        } );                     
                }else if( px > 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,7,false); 
                        } );
                }

                px = x;

                if( px < 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,25,true); 
                        } );                     
                }else if( px > 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,7,true); 
                        } );
                }
            }

            if( y != py ){

                if( py < 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,13,false);
                        } );
                }else if( py > 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,3,false); 
                        } );
                }
                
                py = y;

                if( py < 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,13,true);
                        } );
                }else if( py > 0 ){
                    eventHandlers.push_back( [=](){
                            GPIO::input(1,3,true); 
                        } );
                }
                
            }
        }

	if( e.type == SDL_KEYUP ){
	    switch( e.key.keysym.sym ){
	    case SDLK_ESCAPE:
		hasQuit = true;
		return;
		
	    case SDLK_F5:
		
		eventHandlers.push_back( [](){
			reset();
		    } );
		break;
		
	    case SDLK_F8:
		
		eventHandlers.push_back( [](){
                        if( GDB::connected() ){
                            if( emustate == EmuState::RUNNING )
                                GDB::interrupt();
                        }else{
                            if( emustate == EmuState::RUNNING )
                                emustate = EmuState::STOPPED;
                            else
                                emustate = EmuState::RUNNING;
                        }
		    });

		break;
		
	    case SDLK_F2:
		screenshot = 1;
		SCREEN::dirty = true;
		break;
		
	    case SDLK_F10:
		eventHandlers.push_back([=](){
			std::ofstream os( "dump.img", std::ios::binary );
	
			if( os.is_open() )
			    os.write( reinterpret_cast<char *>(&SD::image[0]), SD::length );
			
		    });
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
		    toggleRecording();
		break;

	    case SDLK_F9: 		
		eventHandlers.push_back( [](){
			GDB::interrupt(); 
		    } );
		break;
	    case SDLK_UP: 		
		eventHandlers.push_back( [=](){
			GPIO::input(1,13,btnState);
			/*
			std::cout << std::hex
				  << int(MMU::read8(0xA0000020+13))
				  << btnState
				  << std::endl;
			*/
		    } ); 
		break;
	    case SDLK_DOWN: 		
		eventHandlers.push_back( [=](){
			GPIO::input(1,3,btnState); 
		    } ); 
		break;
	    case SDLK_LEFT: 		
		eventHandlers.push_back( [=](){
			GPIO::input(1,25,btnState); 
		    } ); 
		break;
	    case SDLK_RIGHT: 		
		eventHandlers.push_back( [=](){
			GPIO::input(1,7,btnState); 
		    } );
		break;
	    case SDLK_a: 		
		eventHandlers.push_back( [=](){
			GPIO::input(1,9,btnState); 
		    } );
		break;
	    case SDLK_s:
	    case SDLK_b: 		
		eventHandlers.push_back( [=](){
			GPIO::input(1,4,btnState); 
		    } );
		break;
	    case SDLK_d:
	    case SDLK_c: 		
		eventHandlers.push_back( [=](){
			GPIO::input(1,10,btnState); 
		    } );
		break;
	    case SDLK_f:
		eventHandlers.push_back( [=](){
			GPIO::input(0,1,btnState); 
		    } );
		break;
		
	    }
	}

    }
}
