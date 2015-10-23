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
    std::map<std::string, uintptr_t> findMap(const std::vector<std::string>& strings, const char *moduleName = nullptr);

private:
    std::map<HMODULE, std::vector<MEMORY_BASIC_INFORMATION>> m_mem;

};



class Wildcard
{
    friend class MaskChar;
public:
    Wildcard(uint8_t len = 1) : m_ch(0xff00 | len) { }
private:
    uint16_t m_ch;
};

class MaskChar
{
public:
    MaskChar(uint8_t ch) : m_ch((uint16_t)ch) { }
    MaskChar(Wildcard wc) : m_ch(wc.m_ch) { }
    bool isMask() const
    {
        return (m_ch & 0xff00) != 0;
    }
    uint8_t getValue() const
    {
        return m_ch & 0x00ff;
    }
private:
    uint16_t m_ch;
};

// Finds a binary pattern with mask in executable sections of a module.
// The mask is a string containing 'x' to match and '?' to ignore.
// If moduleName is nullptr the module of the main module is searched.
uintptr_t FindPattern(const char *byteMask, const char *checkMask, const char *moduleName = nullptr);
// Variant for arbitrary memory.
uintptr_t FindPattern(const char *byteMask, const char *checkMask, uintptr_t address, size_t len);
// More convenient and less error prone alternative.
// Example: hl::FindPattern({ 0x12, 0x45, hl::Wildcard(), 0x89, hl::Wildcard(3), 0xcd, 0xef });
uintptr_t FindPattern(const std::vector<MaskChar>& pattern, const char *moduleName = nullptr);
// Variant for arbitrary memory.
uintptr_t FindPattern(const std::vector<MaskChar>& pattern, uintptr_t address, size_t len);

// Helper to follow relative addresses in instructions.
// Instruction is assumed to end after the relative address. for example jmp, call
uintptr_t FollowRelativeAddress(uintptr_t adr);

MEMORY_BASIC_INFORMATION GetMemoryInfo(const char *moduleName = nullptr);


}

#endif