#pragma once

#include "mmu.hpp"

namespace USART {

    void init();
    void update();
    extern MMU::Layout usart0Layout;

    using Listener = void (*)( u32 v );

}
