
#ifndef __linux__

namespace VERIFY {
    void init(){}
    void update(){}
}

#else

#include <iostream>
#include <cstdio>
#include <numeric>
#include "SDL2/SDL.h"
#include <list>
#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <sstream>

#include "types.hpp"
#include "cpu.hpp"
#include "gdb.hpp"
#include "state.hpp"

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern bool hasQuit;

#define READ 0
#define WRITE 1

pid_t popen2(const char *command, int *infp, int *outfp){
    
    int p_stdin[2], p_stdout[2];
    pid_t pid;

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
	return -1;

    pid = fork();
    if (pid < 0)
	return pid;
    else if (pid == 0)
    {
	close(p_stdin[WRITE]);
	dup2(p_stdin[READ], READ);
	close(p_stdout[READ]);
	dup2(p_stdout[WRITE], WRITE);
	execl("/bin/sh", "sh", "-c", command, NULL);
	perror("execl");
	exit(1);
    }

    if (infp == NULL)
	close(p_stdin[WRITE]);
    else
	*infp = p_stdin[WRITE];
    if (outfp == NULL)
	close(p_stdout[READ]);
    else
	*outfp = p_stdout[READ];
    return pid;
}

namespace VERIFY {
    
    pid_t pid;
    int infp, outfp;
    std::string lastRead;
    std::mutex readMut;
    std::thread worker;

    void pollInput(){
	char buf[257];
	
	while( !hasQuit ){
	    
	    size_t s = ::read(outfp, buf, 256);
	    if( s == -1 ) return;

	    {
		buf[s] = 0;
		std::lock_guard<std::mutex> rml(readMut);
		lastRead += buf;
	    }
	    
	}
    }
    
    std::string read(){
	std::this_thread::sleep_for( std::chrono::milliseconds(150) );
	std::string ret;

	{
	    std::lock_guard<std::mutex> rml(readMut);	
	    ret = lastRead;
	    lastRead.clear();
	}
	return ret;
    }

    std::string send( const std::string &msg ){

	// std::this_thread::sleep_for( std::chrono::milliseconds(100) );
	{
	    std::lock_guard<std::mutex> rml(readMut);	
	    lastRead.clear();
	}

	size_t ws = write(infp, msg.c_str(), msg.size() );
	if( ws < 0 ){
	    pid = 0;
	    return "ERROR";
	}

	if( ws != msg.size() )
	    SDL_Log("BORK");
	
	return read();
    }


    void stop(){
	emustate = EmuState::STOPPED;
    }

    void go(){
	emustate = EmuState::STEP;
	send("si\n");
    }
    
    CPU::reg_pair reg[45];
    bool changes[45];
    bool firstRead = true;

    void readRegisters(){
	for( u32 i=0; i<45; ++i )
	    changes[i] = false;

	std::string acc;
	std::string s = send("i r\n");
	while( !s.empty() ){
	    acc += s;
	    s = read();
	}

	
	acc = acc.substr(acc.find('r'));
	acc = acc.substr(0, acc.size()-7);

	std::stringstream ss(acc);

	u32 rid = 0;
	while( std::getline( ss, s ) ){
	    std::stringstream line(s);
	    std::string regname;
	    uint32_t value;
	    line >> regname >> std::hex >> value;
	    if( reg[rid].I != value ){
		changes[rid] = true;
		// SDL_Log("Register: {{%s}} %s=%x", s.c_str(), regname.c_str(), value);
	    }

	    reg[rid].I = value;
	    
	    rid++;
	}

	if( firstRead ){
	    firstRead = false;
	    for( u32 i=0; i<=16; ++i ){
		CPU::reg[i].I = reg[i].I;
		changes[i] = false;
	    }
	    CPU::CPUUpdateFlags();
	}	

    }

    u32 difCount;
    void compareRegisters(){
	
	CPU::CPUUpdateCPSR();

	bool dif = false;
	for( u32 i=0; i<=16; ++i ){
	    u32 rv = CPU::reg[i].I;
	    if( i == 15 ) rv = CPU::armNextPC&~1;

	    if( reg[i].I != rv && changes[i] ){
		dif = true;
		break;
	    }
	}

	if( !dif ){
	    go();
	    return;
	}

	std::stringstream line;
	line << std::hex << CPU::armNextPC << ": ";

	for( u32 i=0; i<=16; ++i ){
	    u32 rv = CPU::reg[i].I;
	    if( i == 15 ) rv = CPU::armNextPC&~1;

	    if( reg[i].I != rv )
		line << "r"
		     << std::to_string(i)
		     << ": " << CPU::reg[i].I
		     << " != " << reg[i].I
		     << "\n";

	    if( i <= 15 )
		CPU::reg[i].I = reg[i].I;
	    
	    if( i == 15 ){
		CPU::armNextPC = reg[i].I;
		CPU::reg[15].I += 2;
		CPU::THUMB_PREFETCH();
	    }
	}

	SDL_Log( "DIF: %s", line.str().c_str() );

	if( reg[16].I != CPU::reg[16].I && changes[16] )
	    difCount++;

	if( difCount > 20 )
	    stop();
	else
	    go();

    }

    void init(){

	pid = popen2( "arm-none-eabi-gdb -q", &infp, &outfp );
	
	if( !pid )
	    return;

	worker = std::thread(pollInput);
	worker.detach();
	
	emustate = EmuState::JUST_STOPPED;
	SDL_Log("GDB PID %d, RESP %s", pid, read().c_str() );
	SDL_Log("CONNECT: %s", send("target remote :2331\n").c_str());
	std::this_thread::sleep_for( std::chrono::milliseconds(500) );	
	SDL_Log("RESET: %s", send("mon reset 0\n").c_str());
	std::this_thread::sleep_for( std::chrono::milliseconds(500) );
	send("si\n");
	SDL_Log("RESET: %s", send("mon reset 0\n").c_str());
	std::this_thread::sleep_for( std::chrono::milliseconds(500) );
    }

    void update(){
	
	if( !pid || emustate != EmuState::JUST_STOPPED )
	    return;
	
	readRegisters();
	compareRegisters();
	
    }
    
}

#endif
