#include "hacklib/PatternScanner.h"
#include "hacklib/ExeFile.h"
#include <algorithm>


using namespace hl;



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
static void badCharHeuristic(const uint8_t *str, size_t size, int badchar[NO_OF_CHARS])
{
    size_t i;

    // Initialize all occurrences as -1
    for (i = 0; i < NO_OF_CHARS; i++)
        badchar[i] = -1;

    // Fill the actual value of last occurrence of a character
    for (i = 0; i < size; i++)
        badchar[(int)str[i]] = (int)i;
}

/* A pattern searching function that uses Bad Character Heuristic of
Boyer Moore Algorithm */
static const uint8_t *boyermoore(const uint8_t *txt, const size_t n, const uint8_t *pat, const size_t m)
{
    if (m > n || m < 1)
        return nullptr;

    int badchar[NO_OF_CHARS];

    /* Fill the bad character array by calling the preprocessing
    function badCharHeuristic() for given pattern */
    badCharHeuristic(pat, m, badchar);

    int s = 0;  // s is shift of the pattern with respect to text
    int end = (int)(n - m);
    while (s <= end)
    {
        int j = (int)m - 1;

        /* Keep reducing index j of pattern while characters of
        pattern and text are matching at this shift s */
        while (j >= 0 && pat[j] == txt[s + j])
            j--;

        /* If the pattern is present at current shift, then index j
        will become -1 after the above loop */
        if (j < 0)
        {
            // HACKLIB EDIT BEGIN
            // We only want the first occurence of the pattern, so return immediatly.
            return txt + s;

            //printf("\n pattern occurs at shift = %d", s);

            /* Shift the pattern so that the next character in text
            aligns with the last occurrence of it in pattern.
            The condition s+m < n is necessary for the case when
            pattern occurs at the end of text */
            //s += (s + m < n) ? m-badchar[txt[s + m]] : 1;
            // HACKLIB EDIT END
        }
        else
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

// End of third party code.


std::vector<uintptr_t> PatternScanner::find(const std::vector<std::string>& strings, const char *moduleName)
{
    MEMORY_BASIC_INFORMATION mbi = { };
    uintptr_t adr = 0;

    m_mem.clear();

    while (VirtualQuery((LPCVOID)adr, &mbi, sizeof(mbi)) == sizeof(mbi))
    {
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
                const uint8_t *found = boyermoore((const uint8_t*)mbi.BaseAddress, mbi.RegionSize, (const uint8_t*)str.data(), str.size() + 1);

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

    ExeFile exeFile;
    bool verifyWithRelocs = exeFile.loadFromMem((uintptr_t)hMod) && exeFile.hasRelocs();

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
                const uint8_t *baseAdr = (const uint8_t*)mbi.BaseAddress;
                size_t regionSize = mbi.RegionSize;

#ifndef ARCH_64BIT
                do
                {
                    auto found = boyermoore(baseAdr, regionSize, (const uint8_t*)&strAddr, sizeof(uintptr_t));
                    if (found)
                    {
                        // prevent false positives by checking if the reference is relocated
                        if (verifyWithRelocs)
                        {
                            if (!exeFile.isReloc((uintptr_t)found - (uintptr_t)hMod))
                            {
                                // continue searching
                                baseAdr = found + 1;
                                regionSize -= (size_t)(found - baseAdr + 1);
                                continue;
                            }
                        }

                        results[i] = (uintptr_t)found;
                        stringsFound++;
                    }
                } while (false);
#else
                uintptr_t endAdr = (uintptr_t)(baseAdr + regionSize);
                for (uintptr_t adr = (uintptr_t)baseAdr; adr < endAdr; adr++)
                {
                    if (FollowRelativeAddress(adr) == strAddr)
                    {
                        // check for MOV instruction
                        uint8_t opcode = *(uint8_t*)(adr - 3);
                        if (opcode == 0x48) {
                            results[i] = adr;
                            stringsFound++;
                            break;
                        }
                    }
                }
#endif

                if (stringsFound == strings.size())
                    break;

                i++;
            }
            if (stringsFound == strings.size())
                break;
        }
    }

    return results;
}

std::map<std::string, uintptr_t> PatternScanner::findMap(const std::vector<std::string>& strings, const char *moduleName)
{
    auto results = find(strings, moduleName);

    std::map<std::string, uintptr_t> resultMap;
    for (size_t i = 0; i < results.size(); i++)
    {
        resultMap[strings[i]] = results[i];
    }
    return resultMap;
}



static bool compareData(uintptr_t address, const char *byteMask, const char *checkMask)
{
    for (; *checkMask; ++checkMask, ++address, ++byteMask)
        if (*checkMask=='x' && *(char*)address!=*byteMask)
            return false;
    return *checkMask == 0;
}


uintptr_t hl::FindPattern(const char *byteMask, const char *checkMask, const char *moduleName)
{
    auto mbi = GetMemoryInfo(moduleName);
    uintptr_t address = (uintptr_t)mbi.BaseAddress;

    return FindPattern(byteMask, checkMask, address, mbi.RegionSize);
}

uintptr_t hl::FindPattern(const char *byteMask, const char *checkMask, uintptr_t address, size_t len)
{
    uintptr_t end = address + len;
    for (uintptr_t i = address; i < end; i++) {
        if (compareData(i, byteMask, checkMask)) {
            return i;
        }
    }
    return 0;
}

uintptr_t hl::FindPattern(const std::string& pattern, const char *moduleName)
{
    auto mbi = GetMemoryInfo(moduleName);
    uintptr_t address = (uintptr_t)mbi.BaseAddress;

    return FindPattern(pattern, address, mbi.RegionSize);
}

uintptr_t hl::FindPattern(const std::string& pattern, uintptr_t address, size_t len)
{
    std::vector<char> byteMask;
    std::vector<char> checkMask;

    std::string lowPattern = pattern;
    std::transform(lowPattern.begin(), lowPattern.end(), lowPattern.begin(), ::tolower);
    lowPattern += " ";

    for (size_t i = 0; i < lowPattern.size()/3; i++)
    {
        if (lowPattern[3*i+2] == ' ' && lowPattern[3*i] == '?' && lowPattern[3*i+1] == '?')
        {
            byteMask.push_back(0);
            checkMask.push_back('?');
        }
        else if (lowPattern[3*i+2] == ' ' &&
            ((lowPattern[3*i] >= '0' && lowPattern[3*i] <= '9') ||
                (lowPattern[3*i] >= 'a' && lowPattern[3*i] <= 'z')) &&
            ((lowPattern[3*i+1] >= '0' && lowPattern[3*i+1] <= '9') ||
                (lowPattern[3*i+1] >= 'a' && lowPattern[3*i+1] <= 'z')))

        {
            auto value = strtol(lowPattern.data()+3*i, nullptr, 16);
            byteMask.push_back((char)value);
            checkMask.push_back('x');
        }
        else
        {
            throw std::runtime_error("invalid format of pattern string");
        }
    }

    // Terminate mask string, because it is used to determine length.
    checkMask.push_back('\0');

    return FindPattern(byteMask.data(), checkMask.data(), address, len);
}


uintptr_t hl::FollowRelativeAddress(uintptr_t adr)
{
    // Hardcoded 32-bit dereference to make it work with 64-bit code.
    return *(int32_t*)adr + adr + 4;
}


MEMORY_BASIC_INFORMATION hl::GetMemoryInfo(const char *moduleName)
{
    static std::map<const char*, MEMORY_BASIC_INFORMATION> mbi;

    auto it = mbi.find(moduleName);
    if (it != mbi.end())
    {
        return it->second;
    }

    HMODULE modBase = GetModuleHandle(moduleName);
    if (!modBase)
    {
        throw std::runtime_error("no such module");
    }

    uintptr_t codeSectionBase = (uintptr_t)modBase + 0x1000;

    if (!VirtualQuery((LPCVOID)codeSectionBase, &mbi[moduleName], sizeof(MEMORY_BASIC_INFORMATION)))
    {
        throw std::runtime_error("failed to get mbi");
    }

    return mbi[moduleName];
}
