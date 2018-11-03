#include <exception>
#include <string>
#include <regex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include "cpu.hpp"
#include "sys.hpp"
#include "state.hpp"

bool loadBin( const std::string &fileName ){
    FILE *fp = fopen( fileName.c_str(), "rb" );
    if( !fp ) return false;

    struct {
	uint32_t id, size;
    } header;
    uint32_t targetAddr = SYS::vtorReset;

    while( fread( &header, sizeof(header), 1, fp ) == 1 ){
	
	if( header.id > 0x10000000 ){
	    fseek( fp, -8, SEEK_CUR );
	    !fread(
		MMU::flash + targetAddr,
		1,
		sizeof(MMU::flash) - targetAddr,
		fp
		);

	    break;
	}

	fseek( fp, header.size, SEEK_CUR );
	
    }

    fclose(fp);

    return true;
}
