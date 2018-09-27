#include <exception>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>

#include "cpu.hpp"
#include "state.hpp"

bool loadBin( const std::string &fileName ){
    std::ifstream inp( fileName, std::ios::binary );
    if( !inp.good() ) return false;

    struct {
	uint32_t id, size;
    } header;
    uint32_t targetAddr = 0;

    while( !inp.eof() ){
	inp.read( (char *) &header, sizeof(header) );
	if( header.id > 0x10000000 ){
	    inp.seekg( -8, std::ios_base::cur );
	    inp.read(
		(char *) MMU::flash + targetAddr,
		sizeof(MMU::flash) - targetAddr
		);
	    break;
	}
    }

    return true;
}
