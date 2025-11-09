#ifndef HACKLIB_LOGGING_H
#define HACKLIB_LOGGING_H

#include <functional>
#include <stdexcept>
#include <string>
#include <cstdint>


#ifdef _DEBUG
#define HL_LOG_DBG(format, ...)                                                                                        \
    hl::LogDebug(hl::Color::Default, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define HL_LOG_ERR(format, ...) hl::LogError(hl::Color::Red, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define HL_LOG_DBG(format, ...)
#define HL_LOG_ERR(format, ...) hl::LogError(hl::Color::Red, format, ##__VA_ARGS__);
#endif
#define HL_LOG_RAW(format, ...) hl::LogRaw(hl::Color::Default, format, ##__VA_ARGS__);
#define HL_LOG_RAW_COLOR(color, format, ...) hl::LogRaw(color, format, ##__VA_ARGS__);

#ifdef WIN32
#include <Windows.h>
#define HL_ERRCODE ((int)GetLastError())
#else
#include <errno.h>
#define HL_ERRCODE errno
#endif

#define HL_APICHECK(stmt)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(stmt))                                                                                                   \
        {                                                                                                              \
            int code = HL_ERRCODE;                                                                                     \
            HL_LOG_ERR("API check failed: " #stmt "\n");                                                               \
            HL_LOG_ERR("  Error code: %i / 0x%08X / %s\n", code, code, hl::ErrorString(code).c_str());                 \
            throw std::runtime_error("Assert failed: " #stmt);                                                         \
        }                                                                                                              \
    } while (0)


namespace hl
{
void DefaultLogFunc(const std::string& str);

struct LogConfig
{
    // To turn off logging to file.
    bool logToFile = true;
    // Whether or not the time is prepended.
    bool logTime = true;
    // Path of log file. Leave empty for "<moduleName>_log.txt"
    std::string fileName;
    // Log function.
    std::function<void(const std::string&)> logFunc = DefaultLogFunc;
};

enum class Color : uint32_t
{
    Default = 0,

    PlatformBlack = 0x01,
    PlatformRed = 0x02,
    PlatformGreen = 0x03,
    PlatformBlue = 0x04,
    PlatformYellow = 0x05,
    PlatformMagenta = 0x06,
    PlatformCyan = 0x07,
    PlatformWhite = 0x08,

    Black = 0x10,
    Red = 0xC4,
    DarkRed = 0x58,
    LightRed = 0xD9,
    Green = 0x2E,
    DarkGreen = 0x1C,
    LightGreen = 0x9D,
    Blue = 0x15,
    DarkBlue = 0x12,
    LightBlue = 0x21,
    Yellow = 0xE2,
    DarkYellow = 0xD0,
    LightYellow = 0xE5,
    Orange = 0xD0,
    Magenta = 0xC9,
    DarkMagenta = 0x5D,
    LightMagenta = 0xDB,
    Purple = 0x5D,
    Cyan = 0x33,
    DarkCyan = 0x21,
    LightCyan = 0x9F,
    White = 0xE7,
    Gray = 0xF4,
    DarkGray = 0xED,
    LightGray = 0xFA,

    BGBlack = 0x1000,
    BGRed = 0xC400,
    BGDarkRed = 0x5800,
    BGLightRed = 0xD900,
    BGGreen = 0x2E00,
    BGDarkGreen = 0x1C00,
    BGLightGreen = 0x9D00,
    BGBlue = 0x1500,
    BGDarkBlue = 0x1200,
    BGLightBlue = 0x2100,
    BGYellow = 0xE200,
    BGDarkYellow = 0xD000,
    BGLightYellow = 0xE500,
    BGOrange = 0xD000,
    BGMagenta = 0xC900,
    BGDarkMagenta = 0x5D00,
    BGLightMagenta = 0xDB00,
    BGPurple = 0x5D00,
    BGCyan = 0x3300,
    BGDarkCyan = 0x2100,
    BGLightCyan = 0x9F00,
    BGWhite = 0xE700,
    BGGray = 0xF400,
    BGDarkGray = 0xED00,
    BGLightGray = 0xFA00,

    BGPlatformBlack = 0x100,
    BGPlatformRed = 0x200,
    BGPlatformGreen = 0x300,
    BGPlatformBlue = 0x400,
    BGPlatformYellow = 0x500,
    BGPlatformMagenta = 0x600,
    BGPlatformCyan = 0x700,
    BGPlatformWhite = 0x800,

    MaskFG = 0xff,
    MaskBG = 0xff00,
};

inline constexpr hl::Color operator|(hl::Color lhs, hl::Color rhs)
{
    return static_cast<hl::Color>(static_cast<std::underlying_type_t<hl::Color>>(lhs) |
                                  static_cast<std::underlying_type_t<hl::Color>>(rhs));
}
inline constexpr hl::Color operator&(hl::Color lhs, hl::Color rhs)
{
    return static_cast<hl::Color>(static_cast<std::underlying_type_t<hl::Color>>(lhs) &
                                  static_cast<std::underlying_type_t<hl::Color>>(rhs));
}
inline hl::Color& operator|=(hl::Color& lhs, hl::Color rhs)
{
    return lhs = static_cast<hl::Color>(static_cast<std::underlying_type_t<hl::Color>>(lhs) |
                                        static_cast<std::underlying_type_t<hl::Color>>(rhs));
}

void ConfigLog(const LogConfig& config);

void LogDebug(hl::Color color, const char* file, const char* func, int line, const char* format, ...);
void LogError(hl::Color color, const char* file, const char* func, int line, const char* format, ...);
void LogError(hl::Color color, const char* format, ...);
void LogRaw(hl::Color color, const char* format, ...);

std::string ErrorString(int systemCode);
}

#endif
