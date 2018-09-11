#pragma once

enum class EmuState {
    STOPPED,
    JUST_STOPPED,
    RUNNING,
    STEP
};

volatile extern EmuState emustate;
