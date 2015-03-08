#include "hacklib/PageAllocator.h"
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>


static int toUnixProt(int protection)
{
    int unix_prot = PROT_NONE;

    if (protection & hl::PROTECTION_READ)
        unix_prot |= PROT_READ;
    if (protection & hl::PROTECTION_WRITE)
        unix_prot |= PROT_WRITE;
    if (protection & hl::PROTECTION_EXECUTE)
        unix_prot |= PROT_EXEC;
    if (protection & hl::PROTECTION_GUARD)
        throw std::runtime_error("protection not supported");

    return unix_prot;
}


uintptr_t hl::GetPageSize()
{
    static uintptr_t pageSize = 0;

    if (!pageSize)
    {
        pageSize = (uintptr_t)sysconf(_SC_PAGESIZE);
    }

    return pageSize;
}


void *hl::PageAlloc(size_t n, int protection)
{
    return mmap(nullptr, n, toUnixProt(protection), MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

void hl::PageFree(void *p, size_t n)
{
    if (!n)
        throw std::runtime_error("you must specify the free size on linux");

    munmap(p, n);
}

void hl::PageProtect(const void *p, size_t n, int protection)
{
    // Align to page boundary.
    p = (const void*)((uintptr_t)p & ~(hl::GetPageSize() - 1));

    // const_cast: The memory contents will remain unchanged, so this is fine.
    mprotect(const_cast<void*>(p), n, toUnixProt(protection));
}