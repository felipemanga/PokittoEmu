#pragma once

#include <fstream>
#include <memory>
#include <vector>
#include <iostream>

namespace SD {
    extern u32 length;
    extern std::unique_ptr<u8[]> image;
    bool init( const std::string &fileName );
    void write( u32 v );
    extern bool enabled;
}
