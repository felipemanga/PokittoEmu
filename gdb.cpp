#include <iostream>
#include <numeric>
#include <SDL2/SDL_net.h>

#include <list>
#include <thread>
#include <mutex>
#include <chrono>

#include "types.hpp"
#include "cpu.hpp"
#include "gdb.hpp"
#include "state.hpp"

TCPsocket server, client;

static std::list<std::string> inPackets, outPackets;
static std::mutex inmut, outmut;

static bool hasQuit = false;
static std::thread worker;

using lock = std::lock_guard<std::mutex>;

namespace GDB {

    void tx( const std::string &s ){
	SDLNet_TCP_Send( client, s.c_str(), s.size() );
	std::cout << "\n(" << s << ")\n";
    }

    void run(){

	std::string current;
	u8 checksum, sumbytes;
	
	while( !hasQuit ){
	    
	    if( !client ){
		client = SDLNet_TCP_Accept(server);
		if( !client ){
		    std::this_thread::sleep_for( std::chrono::milliseconds(100) );
			continue;
		}
	    }

	    char ch;
	    int len = SDLNet_TCP_Recv( client, &ch, 1 );
	    if( !len ){
		SDLNet_TCP_Close(client);		
		client = 0;
		continue;
	    }

	    std::cout << '[' << ch << ']';

	    if( current.empty() ){

		switch( ch ){
		case '+': // ACK, ignore;
		{
		    lock omg(outmut);

		    if( !outPackets.empty() )
			outPackets.pop_front();

		    continue;
		}

		case '-': // retransmit.
		{
		    lock omg(outmut);

		    if( !outPackets.empty() )
			tx( outPackets.front() );

		    continue;
		}

		case '$':
		    checksum = 0;
		    sumbytes = 0;

		}
		
	    }

	    if( ch == '#' ){
		
		sumbytes = 2;
		
	    }else if( sumbytes ){

		sumbytes--;

		// todo: compare checksum

		if( !sumbytes ){
		    lock img(inmut);
		    inPackets.push_back( current );
		    current.clear();

		    tx("+");
		}

	    }else{
		
		current += ch;
		checksum += ch;
		
	    }
	    

	}
	
    }
        
    bool init( u32 port ){

	if( SDLNet_Init() == -1 )
	    return false;

	emustate = EmuState::STOPPED;

	if( port == 0 )
	    port = 1234;

	IPaddress ip;
	if( SDLNet_ResolveHost( &ip, NULL, port ) == -1 ){
	    printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
	    return false;
	}
	
	server = SDLNet_TCP_Open(&ip);
	if (!server) {
	    printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
	    return false;
	}

	worker = std::thread(run);

	atexit([](){
		hasQuit = true;
		worker.join();
		if( client )
		    SDLNet_TCP_Close(client);
		if( server )
		    SDLNet_TCP_Close(server);
		SDLNet_Quit();
	    });

	return true;

    }

    void write( const std::string &raw ){

	std::string s = "$" + raw + "#";
	u8 checksum = std::accumulate(raw.begin(), raw.end(), 0);

	char hex[3];
	snprintf(hex, 3, "%02x", u32(checksum));
	s += hex;

	lock omg(outmut);
	outPackets.push_back(s);
	if( outPackets.size() == 1 )
	    tx(s);
    }

    bool isCommand( const std::string &cmd, const std::string &search ){
	for( u32 i=0; i<search.size(); ++i ){
	    if( cmd[i+2] != search[i] )
		return false;
	}
	return true;
    }

    /*
    static void sendQuickStatus( u8 signal ){
	char cmd[64];
	u32 cpsr;

	CPU::CPUUpdateCPSR();

	READ_SREG_INTO(g->avr, sreg);

	snprintf(cmd, 64, "T%02x20:%02x;21:%02x%02x;22:%02x%02x%02x00;",
		signal ? signal : 5,
		 CPU::reg[16].I,
		 CPU::reg[13].I,
		 CPU::reg[15].I
	    );
	
	gdb_send_reply(g, cmd);
	
    }    
    */


enum gdb_regnum {
    ARM_A1_REGNUM = 0,            /* first integer-like argument */
    ARM_A4_REGNUM = 3,            /* last integer-like argument */
    ARM_AP_REGNUM = 11,
    ARM_IP_REGNUM = 12,
    ARM_SP_REGNUM = 13,           /* Contains address of top of stack */
    ARM_LR_REGNUM = 14,           /* address to return to from a function call */
    ARM_PC_REGNUM = 15,           /* Contains program counter */
    ARM_F0_REGNUM = 16,           /* first floating point register */
    ARM_F3_REGNUM = 19,           /* last floating point argument register */
    ARM_F7_REGNUM = 23,           /* last floating point register */
    ARM_FPS_REGNUM = 24,          /* floating point status register */
    ARM_PS_REGNUM = 25,           /* Contains processor status */
    ARM_WR0_REGNUM,               /* WMMX data registers.  */
    ARM_WR15_REGNUM = ARM_WR0_REGNUM + 15,
    ARM_WC0_REGNUM,               /* WMMX control registers.  */
    ARM_WCSSF_REGNUM = ARM_WC0_REGNUM + 2,
    ARM_WCASF_REGNUM = ARM_WC0_REGNUM + 3,
    ARM_WC7_REGNUM = ARM_WC0_REGNUM + 7,
    ARM_WCGR0_REGNUM,             /* WMMX general purpose registers.  */
    ARM_WCGR3_REGNUM = ARM_WCGR0_REGNUM + 3,
    ARM_WCGR7_REGNUM = ARM_WCGR0_REGNUM + 7,
    ARM_D0_REGNUM,                /* VFP double-precision registers.  */
    ARM_D31_REGNUM = ARM_D0_REGNUM + 31,
    ARM_FPSCR_REGNUM,
    ARM_NUM_REGS
};
    
    void update(){

	if( !server || inPackets.empty() ) 
	    return;

	lock img(inmut);
	std::string cmd = inPackets.front();
	inPackets.pop_front();

	std::cout << "\nGot: [" << cmd << "]\n";
	switch( cmd[1] ){
	case '?': // query status
	    write("S5");
	    break;

	case 'g':
	{
	    char out[1024]; // > 8*16+2
	    char *outp = out;
	    CPU::CPUUpdateCPSR();
	    u32 i;
	    
	    for( i=0; i<16; ++i, outp += 8 )
		sprintf(outp, "%08x", CPU::reg[i].I);
	    
	    for( ; i<60; ++i, outp += 8 )
		sprintf(outp, "%08x", i);

	    sprintf(outp, "%08x", CPU::reg[16].I);

	    write(out);

	}
	    
	    break;

	case 'H': // set thread id. There is only thread 0.
	    write("OK");
	    break;

	case 'q':
	    
	    if( isCommand(cmd,"Supported") ){
		write("qXfer:memory-map:read+");
		break;
	    }

	    if( isCommand(cmd,"Symbol") ){
		write("OK");
		break;
	    }

	    if( isCommand(cmd,"Attached") ){
		write("1");
		break;
	    }

	    if( isCommand(cmd,"C") ){
		write("");
		break;
	    }

	    if( isCommand(cmd,"fThreadInfo") ){
		write("m-1");
		break;
	    }

	    if( isCommand(cmd,"sThreadInfo") ){
		write("l");
		break;
	    }

	default:
	    write("");
	    return;
	}
	
    }

}
