#pragma once

#include "types.hpp"
#include "mmu.hpp"

namespace SPI {

    extern MMU::Layout spi0Layout, spi1Layout;

    void spi0In( u32 );
    void spi1In( u32 );

    using Listener = void (*)( u32 v );

    void spi0Out( Listener );
    void spi1Out( Listener );
}

