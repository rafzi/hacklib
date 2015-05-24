#include "hacklib/PageAllocator.h"
#include <stdexcept>
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
    case hl::PROTECTION_READ_WRITE:
        windows_prot |= PAGE_READWRITE;
        break;
    case hl::PROTECTION_READ_EXECUTE:
        windows_prot |= PAGE_EXECUTE_READ;
        break;
    case hl::PROTECTION_READ_WRITE_EXECUTE:
        windows_prot |= PAGE_EXECUTE_READWRITE;
        break;
    default:
        throw std::runtime_error("protection not supported");
    }

    return windows_prot;
}


uintptr_t hl::GetPageSize()
{
    static uintptr_t pageSize = 0;

    if (!pageSize)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        pageSize = (uintptr_t)sys_info.dwPageSize;
    }

    return pageSize;
}


void *hl::PageAlloc(size_t n, int protection)
{
    return VirtualAlloc(NULL, n, MEM_COMMIT, toWindowsProt(protection));
}

void hl::PageFree(void *p, size_t n)
{
    VirtualFree(p, 0, MEM_RELEASE);
}

void hl::PageProtect(const void *p, size_t n, int protection)
{
    DWORD dwOldProt;

    // const_cast: The memory contents will remain unchanged, so this is fine.
    VirtualProtect(const_cast<LPVOID>(p), n, toWindowsProt(protection), &dwOldProt);
}