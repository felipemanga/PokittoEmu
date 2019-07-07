#pragma once

class NetManager {
    static int count;

public:
    static void hold();
    static void release();
};
