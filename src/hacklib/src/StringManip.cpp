#include "hacklib/StringManip.h"
#include <algorithm>


bool hl::IsNumber(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

std::string hl::ToUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });

    return s;
}

std::string hl::ToHexDump(std::span<const uint8_t> data, size_t bytesPerLine)
{
    std::string output;

    for (size_t i = 0; i < data.size(); i++)
    {
        // Start new line for every 16 bytes.
        if (i != 0 && i % bytesPerLine == 0)
        {
            output += "\n";
        }
        // Print address row.
        if (i % bytesPerLine == 0)
        {
            char address[8];
            snprintf(address, sizeof(address), "%04X  ", (unsigned int)i);
            output += address;
        }
        // Print byte.
        char byte[4];
        snprintf(byte, sizeof(byte), "%02X ", (unsigned char)data[i]);
        output += byte;

        // Print ASCII row.
        if ((i + 1) % bytesPerLine == 0 || i + 1 == data.size())
        {
            size_t printedInThisRow = (i % bytesPerLine) + 1;
            // Align to end of data row.
            for (size_t j = 0; j < 17 - printedInThisRow; j++)
            {
                output += "   ";
            }
            size_t startOfLine = i & ~0xf;
            for (size_t j = 0; j < printedInThisRow; j++)
            {
                char ascii = data[startOfLine + j];
                if (ascii == '\0')
                {
                    output += ".";
                }
                else if (ascii < 0x20 || ascii == 0x7f)
                {
                    output += ",";
                }
                else if (ascii >= 0x20 && ascii < 0x7f)
                {
                    output += ascii;
                }
                else
                {
                    output += (j % 2 == 0 ? "<" : ">");
                }
            }
        }
    }
    output += "\n";
    return output;
}