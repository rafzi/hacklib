#include "hacklib/ExecutableAllocator.h"
#include <Windows.h>


static DWORD toWindowsProt(int protection)
{
    DWORD windows_prot = 0;

    if (protection & hl::PROTECTION_GUARD)
    {
        protection &= ~hl::PROTECTION_GUARD;
        windows_prot |= PAGE_GUARD;
    }

    switch (protection)
    {
    case 0:
        windows_prot |= PAGE_NOACCESS;
        break;
    case hl::PROTECTION_READ:
        windows_prot |= PAGE_READONLY;
        break;
    case hl::PROTECTION_READ|hl::PROTECTION_WRITE:
        windows_prot |= PAGE_READWRITE;
        break;
    case hl::PROTECTION_READ|hl::PROTECTION_EXECUTE:
        windows_prot |= PAGE_EXECUTE_READ;
        break;
    case hl::PROTECTION_READ|hl::PROTECTION_WRITE|hl::PROTECTION_EXECUTE:
        windows_prot |= PAGE_EXECUTE_READWRITE;
        break;
    default:
        throw std::runtime_error("protection not supported");
    }

    return windows_prot;
}


void *hl::PageAlloc(size_t n, int protection)
{
    return VirtualAlloc(NULL, n, MEM_COMMIT, toWindowsProt(protection));
}

void hl::PageFree(void *p)
{
    VirtualFree(p, 0, MEM_RELEASE);
}

void hl::PageProtect(const void *p, size_t n, int protection)
{
    DWORD dwOldProt;

    // const_cast: The memory contents will remain unchanged, so this is fine.
    VirtualProtect(const_cast<LPVOID>(p), n, toWindowsProt(protection), &dwOldProt);
}