#include "hacklib/PatternScanner.h"
#include <algorithm>


static std::map<const char*, MEMORY_BASIC_INFORMATION> s_mbi;



static HMODULE GetModuleFromAddress(uintptr_t adr)
{
    HMODULE hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)adr,
        &hModule);

    return hModule;
}



// taken from: http://www.geeksforgeeks.org/pattern-searching-set-7-boyer-moore-algorithm-bad-character-heuristic/
/* Program for Bad Character Heuristic of Boyer Moore String Matching Algorithm */

#include <climits>
#include <cstdint>

#define NO_OF_CHARS 256

// The preprocessing function for Boyer Moore's bad character heuristic
void badCharHeuristic(const uint8_t *str, int size, int badchar[NO_OF_CHARS])
{
    int i;

    // Initialize all occurrences as -1
    for (i = 0; i < NO_OF_CHARS; i++)
        badchar[i] = -1;

    // Fill the actual value of last occurrence of a character
    for (i = 0; i < size; i++)
        badchar[(int)str[i]] = i;
}

/* A pattern searching function that uses Bad Character Heuristic of
Boyer Moore Algorithm */
const uint8_t *boyermoore(const uint8_t *txt, const int n, const uint8_t *pat, const int m)
{
    int badchar[NO_OF_CHARS];

    /* Fill the bad character array by calling the preprocessing
    function badCharHeuristic() for given pattern */
    badCharHeuristic(pat, m, badchar);

    int s = 0;  // s is shift of the pattern with respect to text
    int end = n - m;
    while (s <= end)
    {
        int j = m - 1;

        /* Keep reducing index j of pattern while characters of
        pattern and text are matching at this shift s */
        while (j >= 0 && pat[j] == txt[s + j])
            j--;

        /* If the pattern is present at current shift, then index j
        will become -1 after the above loop */
        if (j < 0)
        {
            return txt + s;
            printf("\n pattern occurs at shift = %d", s);

            /* Shift the pattern so that the next character in text
            aligns with the last occurrence of it in pattern.
            The condition s+m < n is necessary for the case when
            pattern occurs at the end of text */
            s += (s + m < n) ? m-badchar[txt[s + m]] : 1;
        } else
        {
            /* Shift the pattern so that the bad character in text
            aligns with the last occurrence of it in pattern. The
            max function is used to make sure that we get a positive
            shift. We may get a negative shift if the last occurrence
            of bad character in pattern is on the right side of the
            current character. */
            s += std::max(1, j - badchar[txt[s + j]]);
        }
    }

    return nullptr;
}


std::vector<uintptr_t> PatternScanner::find(const std::vector<std::string>& strings, const char *moduleName)
{
    MEMORY_BASIC_INFORMATION mbi = { };
    uintptr_t adr = 0;

    m_mem.clear();

    while (adr < 0x7FFF0000)
    {
        if (!VirtualQuery((LPCVOID)adr, &mbi, sizeof(mbi)))
            throw std::runtime_error("VirtualQuery failed");

        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_IMAGE)
        {
            HMODULE mod = GetModuleFromAddress(adr);
            if (mod)
                m_mem[mod].push_back(mbi);
        }

        adr += mbi.RegionSize;
    }

    HMODULE hMod = GetModuleHandle(moduleName);

    std::vector<uintptr_t> strAddrs(strings.size());
    int stringsFound = 0;

    // search all readonly sections for the strings
    for (const auto& mbi : m_mem[hMod])
    {
        if (mbi.Protect == PAGE_READONLY)
        {
            int i = 0;
            for (const auto& str : strings)
            {
                const uint8_t *found = boyermoore((const uint8_t*)mbi.BaseAddress, mbi.RegionSize, (const uint8_t*)str.data(), str.size());

                if (found)
                {
                    strAddrs[i] = (uintptr_t)found;
                    stringsFound++;
                    if (stringsFound == strings.size())
                        break;
                }
                i++;
            }
            if (stringsFound == strings.size())
                break;
        }
    }

    if (stringsFound != strings.size())
        throw std::runtime_error("One or more patterns not found");

    std::vector<uintptr_t> results(strings.size());
    stringsFound = 0;

    // search all code sections for references to the strings
    for (const auto& mbi : m_mem[hMod])
    {
        if (mbi.Protect == PAGE_EXECUTE_READ)
        {
            int i = 0;
            for (const auto& strAddr : strAddrs)
            {
                const uint8_t *found = boyermoore((const uint8_t*)mbi.BaseAddress, mbi.RegionSize, (const uint8_t*)&strAddr, sizeof(uintptr_t));

                if (found)
                {
                    results[i] = (uintptr_t)found;
                    stringsFound++;
                    if (stringsFound == strings.size())
                        break;
                }
                i++;
            }
            if (stringsFound == strings.size())
                break;
        }
    }

    return results;
}



bool compareData(uintptr_t address, const char *byteMask, const char *checkMask)
{
    for (; *checkMask; ++checkMask, ++address, ++byteMask)
        if (*checkMask=='x' && *(char*)address!=*byteMask)
            return false;
    return *checkMask == 0;
}


uintptr_t FindPattern(const char *byteMask, const char *checkMask, const char *moduleName)
{
    uintptr_t& address = (uintptr_t&)s_mbi[moduleName].BaseAddress;

    if (!address)
    {
        HMODULE modBase = GetModuleHandle(moduleName);
        if (!modBase)
            return 0;

        uintptr_t codeSectionBase = (uintptr_t)modBase + 0x1000;

        if (!VirtualQuery((LPCVOID)codeSectionBase, &s_mbi[moduleName], sizeof(MEMORY_BASIC_INFORMATION)))
            return 0;
    }

    uintptr_t end = address + s_mbi[moduleName].RegionSize;
    for (uintptr_t i = address; i < end; i++) {
        if (compareData(i, byteMask, checkMask)) {
            return i;
        }
    }
    return 0;
}


std::uintptr_t FollowRelativeAddress(std::uintptr_t adr)
{
    return *(uintptr_t*)adr + adr + 4;
}