#pragma once

enum class EmuState {
    STOPPED,
    JUST_STOPPED,
    RUNNING,
    STEP
};

extern EmuState emustate;
