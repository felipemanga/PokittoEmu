#include "types.hpp"
#include <string>

// namespace AUDIO {
//     void write(u8 data){}
// }

namespace PEX {
    bool init(u32 port){ return true; }
    u32 update(){return 0;}
}

bool convertFile(const std::string& zipFileName){return false;}
