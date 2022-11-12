#pragma once

namespace AUDIO {
    bool init(u32 hz);
    void write(u8 data);
    void update();
    void checkHLE(u32 rate);
}
