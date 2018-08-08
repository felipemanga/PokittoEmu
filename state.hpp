#pragma once

enum class EmuState {
    STOPPED,
    RUNNING,
    STEP
};

extern EmuState emustate;
