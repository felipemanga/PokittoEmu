#pragma once

namespace GDB {
    bool init( u32 port );
    void update();
    bool connected();
    void interrupt();
}
