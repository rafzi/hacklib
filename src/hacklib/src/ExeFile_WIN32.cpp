#include "hacklib/ExeFile.h"


bool hl::ExeFile::loadElf(uintptr_t)
{
    // Reading ELF images needs <elf.h>, which only UNIX provides.
    return false;
}
