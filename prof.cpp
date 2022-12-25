
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

#endif

#include "types.hpp"
#include "cpu.hpp"

namespace PROF {
    u32 hits[ sizeof(MMU::flash)>>1 ];
    const u32 hitCount = sizeof(hits)/sizeof(hits[0]);
    bool dumpOnExit = true;
    bool hitCaller;

    struct Sample{
	u32 addr;
	u32 count;
    };

    static void dump() {
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
	for( u32 i=0, max = std::min<u32>(samples.size(), 128); i<max; ++i ){
	    out << std::hex << samples[i].addr << "  " << samples[i].count << std::endl;
	}
    }

    void init( bool _hitCaller ){
	hitCaller = _hitCaller;
	atexit([](){
            if( dumpOnExit )
                dump();
        });
    }
}
