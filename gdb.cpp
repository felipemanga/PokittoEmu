#ifdef __EMSCRIPTEN__

namespace GDB {

    bool connected(){
	return false;
    }

    void interrupt(){
    }
    
}

#else

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
#include "state.hpp"

TCPsocket server, client;

static std::list<std::string> inPackets, outPackets;
static std::mutex inmut, outmut, txmut;
extern bool hasQuit;
static bool noAckMode = false;
static std::thread worker;
extern u32 verbose;
using lock = std::lock_guard<std::mutex>;

char out[0x4000];

u32 rev32( u32 i ){
    return ((i<<24) | (i>>24) | ((i&0xff00) << 8) | ((i>>8) & 0xff00 ));
}

namespace GDB {

    static bool interruptRequested = false;

    void tx( std::string s ){
	lock tml(txmut);

	if( verbose>1 ) std::cout << "\n<<(" << s << ")\n";
	SDLNet_TCP_Send( client, s.c_str(), s.size() );
	/*
	std::cout << "\n(";
	for( auto i : s )
	    std::cout << u32(i) << " ";
	std::cout << s << ")\n";
	*/
	
    }

    void run(){

	std::string current;
	u8 checksum, sumbytes;
	
	while( !hasQuit ){
	    
	    if( !client ){
		client = SDLNet_TCP_Accept(server);
		noAckMode = false;
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

	    if( verbose>1 ) std::cout << '[' << ch << ']';

	    {
		lock omg(outmut);
		outPackets.clear();
	    }

	    if( current.empty() ){

		switch( ch ){
		case 3:
		    interruptRequested = true;
		    continue;

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
		    else
			tx("+");

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

		    if( !noAckMode )
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
	std::snprintf(hex, 3, "%02x", u32(checksum));
	s += hex;

	if( noAckMode ){
	    tx(s);
	}else{
	    lock omg(outmut);
	    outPackets.push_back(s);
	    if( outPackets.size() == 1 )
		tx(s);
	}
    }

    bool isCommand( const std::string &cmd, const std::string &search ){
	for( u32 i=0; i<search.size(); ++i ){
	    if( cmd[i+2] != search[i] )
		return false;
	}
	return true;
    }

    bool connected(){
	return !!client;
    }

    void interrupt(){
	emustate = EmuState::JUST_STOPPED;
	CPU::cpuNextEvent = CPU::cpuTotalTicks;
    }

    void update(){

	if( !server ) return;

        if( interruptRequested ){
            interruptRequested = false;
            interrupt();
        }
        
	if( emustate == EmuState::JUST_STOPPED ){
	    write("T05");
	    emustate = EmuState::STOPPED;
	}
	
	if( inPackets.empty() ) return;

	lock img(inmut);
	std::string cmd = inPackets.front();
	inPackets.pop_front();

	// std::cout << "\nGot: [" << cmd << "]\n";
	switch( cmd[1] ){
	case 'c': // continue;
	    emustate = EmuState::RUNNING;
	    write("OK");
	    break;

	case 's': // step
	    emustate = EmuState::STEP;
	    write("OK");
	    break;

	case '?': // query status
	    write("S05");
	    break;

	case 'k': // kill
	    hasQuit = true;
	    break;

	case 'g':
	{
	    char *outp = out;
	    u32 i;

	    for( i=0; i<16; ++i, outp += 8 ){
		if( i == 15 )
		    std::sprintf(outp, "%08x", rev32(CPU::armNextPC&~1));
		else
		    std::sprintf(outp, "%08x", rev32(CPU::reg[i].I) );
	    }

	    for( i=0; i<12; ++i, outp += 16 )
		std::sprintf(outp, "0000000000000000");

	    std::sprintf(outp, "00000000"); outp+=8; // FPSR

	    write(out);
	    
	    break;
	}

	case 'p':
	{
	    u32 i = strtol( cmd.c_str()+2, nullptr, 16 );
	    if( i == 0x19 ){
		i = 16;
		CPU::CPUUpdateCPSR();
	    }
	    std::sprintf(out, "%08x", rev32(CPU::reg[i].I));
	    write(out);
	    break;
	}

	case 'm': // read memory
	{
	    u32 addr, len;
	    addr = strtoul( cmd.c_str() + 2, nullptr, 16 );
	    len  = strtoul( cmd.c_str() + 1 + cmd.find(','), nullptr, 16 );
	    char *outp = out;
	    // std::cout << std::hex << addr << ", " << len << std::endl;
	    for( u32 i=0; i<len; ++i, outp += 2 ){
		if( addr+i > 0x30000000 ) break;
		std::sprintf( outp, "%02x", MMU::read8( addr+i ) );
	    }
	    
	    write( out );
	}
	    break;

	case 'M': // write memory
	{
	    u32 addr, len, start = cmd.find(':') + 1;
	    addr = strtoul( cmd.c_str() + 2, nullptr, 16 );
	    // len  = strtoul( cmd.c_str() + 1 + cmd.find(','), nullptr, 16 )*2;
	    len = cmd.size() - start;
	    char tmp[3];
	    tmp[2] = 0;
	    
	    // std::cout << std::hex << addr << ", " << len << std::endl;
	    for( u32 i=0; i<len-1; i+=2, ++addr ){
		tmp[0] = cmd[start+i];
		tmp[1] = cmd[start+i+1];
		u8 b = strtol(tmp, nullptr,16);
		// std::cout << addr << " = " << u32(MMU::flash[addr]) << " -> " << u32(b) << std::endl;
		
		if( addr > 0x30000000 ) break;
		if( addr < sizeof(MMU::flash) ){
		    MMU::flash[addr] = b;
		}else
		    MMU::write8( addr++, b );
	    }

	    CPU::THUMB_PREFETCH();
	    
	    write("OK");
	}
	    break;
	    
	case 'D':
	    write("OK");
	    emustate = EmuState::RUNNING;
	    break;

	case 'H': // set thread id. There is only thread 0.
	    write("OK");
	    break;

	case 'Q':
	    if( isCommand(cmd, "StartNoAckMode") ){
		noAckMode=true;
		write("OK");
		break;
	    }

	    write("");
	    break;

	case 'q':

	    if( isCommand(cmd,"Rcmd,7265736574") ){
		std::cout << "Reset command" << std::endl;
		CPU::reset();
		write("OK");		
		break;
	    }
	    
	    if( isCommand(cmd,"Supported") ){
		write("PacketSize=3fff;qXfer:memory-map:read+;QStartNoAckMode+");
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

	    if( isCommand(cmd,"Xfer:memory-map:read") ) {
	        write(
		    "l<memory-map>\n"
		    " <memory type='ram' start='0x20004000' length='0x800'/>\n"
		    " <memory type='ram' start='0x20000000' length='0x800'/>\n"
		    " <memory type='ram' start='0x10000000' length='0x8000'/>\n"
		    " <memory type='ram' start='0' length='0x40000'/>"
		    "</memory-map>"
		    );
		break;
	    }
/*
	    if( isCommand(cmd,"fThreadInfo") ){
		write("m-1");
		break;
	    }

	    if( isCommand(cmd,"sThreadInfo") ){
		write("l");
		break;
	    }
*/
	default:
	    write("");
	    return;
	}
	
    }

}

#endif
