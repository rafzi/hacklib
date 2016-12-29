#ifndef HACKLIB_PATTERNSCANNER_H
#define HACKLIB_PATTERNSCANNER_H

#include "hacklib/Memory.h"
#include "hacklib/ExeFile.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cstdint>


namespace hl {


class PatternScanner
{
public:
    // Searches for referenced strings in code of the module.
    std::vector<uintptr_t> find(const std::vector<std::string>& strings, const std::string& moduleName = "");
    std::map<std::string, uintptr_t> findMap(const std::vector<std::string>& strings, const std::string& moduleName = "");

    static std::unordered_map<std::string, hl::ModuleHandle> moduleMap;
    static std::unordered_map<std::string, std::unique_ptr<hl::ExeFile>> exeFileMap;
    static std::unordered_map<std::string, bool> verifyRelocsMap;
};


// Finds a binary pattern with mask in executable sections of a module.
// The mask is a string containing 'x' to match and '?' to ignore.
// If moduleName is nullptr the module of the main module is searched.
uintptr_t FindPatternMask(const char *byteMask, const char *checkMask, const std::string& moduleName = "");
// Variant for arbitrary memory.
uintptr_t FindPatternMask(const char *byteMask, const char *checkMask, uintptr_t address, size_t len);
// More convenient and less error prone alternative.
// Example: "12 45 ?? 89 ?? ?? ?? cd ef"
uintptr_t FindPattern(const std::string& pattern, const std::string& moduleName = "");
uintptr_t FindPattern(const std::string& pattern, uintptr_t address, size_t len);

// Helper to follow relative addresses in instructions.
// Instruction is assumed to end after the relative address. for example jmp, call
uintptr_t FollowRelativeAddress(uintptr_t adr);

const std::vector<hl::MemoryRegion>& GetCodeRegions(const std::string& moduleName = "");
}

#endif
