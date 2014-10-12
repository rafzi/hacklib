#include "hacklib/Main.h"

#include <cstdio>
class MyMain : hl::Main
{
    bool step() override
    {
        static int i = 0;
        printf("asdasdas\n");
        if (i++ > 10) return false;
        return true;
    }
};


MyMain g_main;