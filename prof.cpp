#if !defined(__EMSCRIPTEN__)

#include <iostream>
#include <cstdio>
#include <numeric>
#include <SDL2/SDL.h>
#include <fstream>

#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

#include "types.hpp"
#include "cpu.hpp"

extern volatile bool hasQuit;

namespace PROF {
    u32 lastTick;
    u32 hits[ sizeof(MMU::flash)>>1 ];
    bool hitCaller;

    static std::thread worker;

    struct Sample{
	u32 addr;
	u32 count;
    };
    
    void run(){

	const u32 hitCount = sizeof(hits)/sizeof(hits[0]);

	std::cout << "Profiling..." << std::endl;

	while( !hasQuit ){
	    
	    if( CPU::cpuTotalTicks == lastTick ){
		std::this_thread::sleep_for( std::chrono::milliseconds(1) );
		continue;
	    }

	    u32 delta = CPU::cpuTotalTicks - lastTick;

	    lastTick = CPU::cpuTotalTicks;
	    
	    u32 addr = CPU::ADDRESS >> 1;
	    if( addr < hitCount )
		hits[addr]++;

	    if( hitCaller ){
		addr = (CPU::reg[14].I >> 1)-2;
		if( addr < hitCount )
		    hits[addr]++;
	    }
	    
	}

	std::cout << "Preparing samples..." << std::endl;

	u32 maxCount = 0;
	std::vector<Sample> samples;
	for( u32 i=0; i<hitCount; ++i ){
	    if( !hits[i] ) 
		continue;

	    if( hits[i] > maxCount ) 
		maxCount = hits[i];

	    samples.push_back(Sample{ i<<1, hits[i] });
	}

	std::sort(
	    samples.begin(),
	    samples.end(),
	    []( const Sample &l, const Sample &r ) -> bool {
		return r.count < l.count;
	    });

	std::ofstream out("hotspots.log");
	for( u32 i=0; i<20; ++i ){
	    out << std::hex << samples[i].addr << std::endl;
	}

    }

    void init( bool _hitCaller ){
	hitCaller = _hitCaller;
	worker = std::thread(run);
	atexit([](){
		hasQuit = true;
		worker.join();
	    });
    }
}

#endif
