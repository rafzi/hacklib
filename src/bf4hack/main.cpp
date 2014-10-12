#include "hacklib/Main.h"


// class definitions


class MyMain : hl::Main
{
    bool init() override
    {
        // patternscan
        // hook
        // overlay
        // drawer
        return true;
    }

    bool step() override
    {
        // wait for close
        return true;
    }

    void shutdown() override
    {
        // undo stuff
    }
};


MyMain g_main;