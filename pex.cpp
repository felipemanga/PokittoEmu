#include <iostream>
#include <cstdio>
#include <numeric>
#include <SDL2/SDL_net.h>

#include <list>
#include <thread>
#include <mutex>
#include <chrono>

#include "types.hpp"
#include "cpu.hpp"
#include "gdb.hpp"
#include "pex.hpp"
#include "state.hpp"
#include "sdl.hpp"
#include "net.hpp"
#include "gpio.hpp"

u32 samplingFrequency = 48 * 1024 * 1024 >> 4;
static TCPsocket server, listener;
extern bool hasQuit;
static std::thread worker;
u64 lastTick;
s64 tti;

class Client {
    TCPsocket socket;
    std::thread thread;

public:
    Client(TCPsocket socket) : socket(socket), thread(&Client::run, this) {
    }

    virtual ~Client(){
        thread.join();
        if(socket)
            SDLNet_TCP_Close(socket);
    }

    void run(){
        std::string buffer;
        char command = 0;

        while( !hasQuit ){

            char ch;
            int len = SDLNet_TCP_Recv( socket, &ch, 1 );
            if( !len ){
                if( listener == socket )
                    listener = 0;
                SDLNet_TCP_Close(socket);
                socket = 0;
                return;
            }

            switch( command ){
            case 0:
                command = ch;
                continue;

            case 'l':
                command = 0;
                listener = socket;
                continue;

            case 'g':
                command = 0;
                GDB::interrupt();
                continue;
            
            case 'f':
                if(ch==';'){
                    try{
                        samplingFrequency = std::stoi(buffer);
                    }catch(std::exception ex){
                        std::cout << "Invalid Frequency:" << buffer << std::endl;
                    }
                    command = 0;
                    buffer.clear();
                }else{
                    buffer += ch;
                }
                break;
            }
            
        }
    }
};

std::list<Client> clients;

static void sample(){
    if( !listener ) return;

    u32 out[] = { GPIO::POUT0, GPIO::POUT1, GPIO::POUT2 };
    SDLNet_TCP_Send( listener, (char*)out, 4*3 );

}

static void run(){
    while( !hasQuit ){

        TCPsocket clientSocket = SDLNet_TCP_Accept(server);
        if( !clientSocket ){
            std::this_thread::sleep_for( std::chrono::milliseconds(100) );
            continue;
        }
        clients.emplace_back(clientSocket);

    }
}

namespace PEX {

    bool init( u32 port ){
        NetManager::hold();
	atexit([](){
		hasQuit = true;
		worker.join();
		if( server )
		    SDLNet_TCP_Close(server);
                clients.empty();
                NetManager::release();
	    });

	IPaddress ip;
	if( SDLNet_ResolveHost( &ip, nullptr, port ) == -1 ){
	    std::printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
	    return false;
	}
	
	server = SDLNet_TCP_Open(&ip);
	if (!server) {
	    std::printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
	    return false;
	}

	worker = std::thread(run);

        return true;
    }

    u32 update(){
        u32 delta = CPU::cpuTotalTicks - lastTick;
        lastTick = CPU::cpuTotalTicks;
        if( !listener )
            return ~0;
        
        tti -= delta;
        if( tti < 0 ){
            sample();
            tti += samplingFrequency;
            if( tti < 0 )
                tti = samplingFrequency;
        }
        return tti;
    }

}
