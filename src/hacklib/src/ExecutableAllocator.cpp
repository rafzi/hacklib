#include "hacklib/ExecutableAllocator.h"
#include <Windows.h>


void *hl::ExecutableAlloc(size_t n)
{
    return VirtualAlloc(NULL, n, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void hl::ExecutableFree(void *p)
{
    VirtualFree(p, 0, MEM_RELEASE);
}