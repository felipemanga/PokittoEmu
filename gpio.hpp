#pragma once

#include "mmu.hpp"

namespace GPIO {

    void init();

    extern MMU::Layout byteLayout, wordLayout, mainLayout;

}
