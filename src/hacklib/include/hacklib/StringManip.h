#ifndef HACKLIB_STRINGMANIP_H
#define HACKLIB_STRINGMANIP_H

#include <string>
#include <iomanip>
#include <sstream>
#include <span>
#include <cstdint>


namespace hl
{

// Converts a number to a hexadecimal string. If width is zero, the string is padded with zeroes to the size of the type
// of num.
template <typename T>
static std::string ToHexStr(T num, int width = 0)
{
    std::stringstream result;
    result << "0x" << std::hex << std::setfill('0') << std::setw(width ? width : sizeof(T) * 2) << (num | 0);
    return result.str();
}

// Checks if the given string represents a number.
bool IsNumber(const std::string& s);

// Convert a string to uppercase.
std::string ToUpper(std::string s);

std::string ToHexDump(std::span<const uint8_t> data, size_t bytesPerLine = 16);

}

#endif
