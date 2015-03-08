#include "hacklib/Hooker.h"
#include <thread>
#include <chrono>
#include <iostream>

#pragma comment(linker, "/INCREMENTAL:NO")


void dummy()
{
    printf("dummy called\n");

    int i = 5;
    int j = 7;
    int k = i * j;
    int l = k << i;
}


void hkFunc()
{
    printf("hooked!\n");
    for (;;);
}


void hkDetour(hl::CpuContext *ctx)
{
    printf("got context!\n");
}



int main()
{
    hl::Hooker hooker;

    //hooker.hookJMP((uintptr_t)dummy, 14, hkFunc);

#ifdef ARCH_64BIT
    hooker.hookDetour((uintptr_t)dummy, 14, hkDetour);
#else
    hooker.hookDetour((uintptr_t)dummy, 6, hkDetour);
#endif


    dummy();

    printf("execution returned to main\n");
    std::cin.ignore();
}