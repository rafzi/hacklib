#ifndef PATTERNSCANNER_H
#define PATTERNSCANNER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <Windows.h>


namespace hl {


class PatternScanner
{
public:
    // Searches for referenced strings in code of the module.
    std::vector<uintptr_t> find(const std::vector<std::string>& strings, const char *moduleName = nullptr);

private:
    std::map<HMODULE, std::vector<MEMORY_BASIC_INFORMATION>> m_mem;

};



std::uintptr_t FindPattern(const char *byteMask, const char *checkMask, const char *moduleName = nullptr);

// instruction is assumed to end after the relative address. for example jmp, call
std::uintptr_t FollowRelativeAddress(std::uintptr_t adr);


}

#endif