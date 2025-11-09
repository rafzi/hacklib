#include "hacklib/Logging.h"
#include "hacklib/Main.h"
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>


static constexpr auto ANSIESC_RESET = "\x1b[0m";
static constexpr auto ANSIESC_COLOR_RED = "\x1b[31m";
static constexpr auto ANSIESC_COLOR_GREEN = "\x1b[32m";
static constexpr auto ANSIESC_COLOR_YELLOW = "\x1b[33m";
static constexpr auto ANSIESC_COLOR_BLUE = "\x1b[34m";
static constexpr auto ANSIESC_COLOR_PURPLE = "\x1b[35m";
static constexpr auto ANSIESC_COLOR_CYAN = "\x1b[36m";
static constexpr auto ANSIESC_COLOR_WHITE = "\x1b[37m";
static constexpr auto ANSIESC_BG_COLOR_RED = "\x1b[41m";
static constexpr auto ANSIESC_BG_COLOR_GREEN = "\x1b[42m";
static constexpr auto ANSIESC_BG_COLOR_YELLOW = "\x1b[43m";
static constexpr auto ANSIESC_BG_COLOR_BLUE = "\x1b[44m";
static constexpr auto ANSIESC_BG_COLOR_PURPLE = "\x1b[45m";
static constexpr auto ANSIESC_BG_COLOR_CYAN = "\x1b[46m";
static constexpr auto ANSIESC_BG_COLOR_WHITE = "\x1b[47m";
static constexpr auto ANSIESC_COLOR_8BIT_PREFIX = "\x1b[38;5;";
static constexpr auto ANSIESC_BG_COLOR_8BIT_PREFIX = "\x1b[48;5;";


hl::LogConfig& getGlobalConfig()
{
    static hl::LogConfig cfg;
    return cfg;
}


class FormatStr
{
public:
    FormatStr(const char* format, va_list vl)
    {
        va_list vl_copy;
        va_copy(vl_copy, vl);

        const int size = vsnprintf(nullptr, 0, format, vl);
        m_str = new char[size + 1];
        vsnprintf(m_str, size + 1, format, vl_copy);

        va_end(vl_copy);
    }
    FormatStr(const FormatStr&) = delete;
    FormatStr& operator=(const FormatStr&) = delete;
    FormatStr(FormatStr&&) = delete;
    FormatStr& operator=(FormatStr&&) = delete;
    ~FormatStr() { delete[] m_str; }
    [[nodiscard]] const char* str() const { return m_str; }

private:
    char* m_str;
};


static std::string GetTimeStr()
{
    char strtime[256];
    auto timept = std::chrono::system_clock::now();
    auto timetp = std::chrono::system_clock::to_time_t(timept);
    strftime(strtime, sizeof(strtime), "%X", std::localtime(&timetp));

    return strtime;
}

static std::string GetCodeStr(const char* file, const char* func, int line)
{
    std::string fileStr = file;
    fileStr = fileStr.substr(fileStr.find_last_of('\\') + 1);
    fileStr = fileStr.substr(fileStr.find_last_of('/') + 1);

    std::string str;
    str += '[';
    str += fileStr;
    str += ':';
    str += std::to_string(line);
    str += '|';
    str += func;
    str += ']';
    return str;
}

static void LogString(const std::string& str, bool raw, hl::Color color)
{
    const auto& cfg = getGlobalConfig();

    std::string logStr;
    if (!raw && cfg.logTime)
    {
        logStr = GetTimeStr() + " " + str;
    }
    else
    {
        logStr = str;
    }

    if (cfg.logToFile)
    {
        std::ofstream logfile(cfg.fileName, std::ios::out | std::ios::app);
        logfile << logStr;
    }

    if (cfg.logFunc)
    {
        std::string colorPrefix;
        auto fgColor = color & hl::Color::MaskFG;
        auto bgColor = color & hl::Color::MaskBG;
        bool requiresReset = false;
        if (fgColor != hl::Color::Default)
        {
            colorPrefix += ANSIESC_COLOR_8BIT_PREFIX;
            auto raw = static_cast<uint32_t>(fgColor);
            if (raw < 0x10)
                raw -= 1;
            colorPrefix += std::to_string(raw) + 'm';
            requiresReset = true;
        }
        if (bgColor != hl::Color::Default)
        {
            colorPrefix += ANSIESC_BG_COLOR_8BIT_PREFIX;
            auto raw = static_cast<uint32_t>(bgColor) >> 8;
            if (raw < 0x10)
                raw -= 1;
            colorPrefix += std::to_string(raw) + 'm';

            // Avoid buggy colors on line breaks.
            std::string sanitizedLogStr;
            size_t lastPos = 0;
            while (lastPos < logStr.size())
            {
                size_t pos = logStr.find('\n', lastPos);
                if (pos == std::string::npos)
                {
                    sanitizedLogStr += colorPrefix + logStr.substr(lastPos);
                    requiresReset = true;
                    break;
                }
                sanitizedLogStr += colorPrefix + logStr.substr(lastPos, pos - lastPos) + ANSIESC_RESET + '\n';
                requiresReset = false;
                lastPos = pos + 1;
            }
            logStr = std::move(sanitizedLogStr);
        }
        else
        {
            logStr = colorPrefix + logStr;
        }
        if (requiresReset)
        {
            logStr += ANSIESC_RESET;
        }
        cfg.logFunc(logStr);
    }
}


void hl::DefaultLogFunc(const std::string& str)
{
    fputs(str.c_str(), stdout);
}


void hl::ConfigLog(const LogConfig& config)
{
    auto& cfg = getGlobalConfig();
    cfg = config;

    if (cfg.fileName == "")
    {
        cfg.fileName = hl::GetCurrentModulePath() + "_log.txt";
    }
}


void hl::LogDebug(hl::Color color, const char* file, const char* func, int line, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(GetCodeStr(file, func, line) + " " + str.str(), false, color);

    va_end(vl);
}

void hl::LogError(hl::Color color, const char* file, const char* func, int line, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(GetCodeStr(file, func, line) + " ERROR: " + str.str(), false, color);

    va_end(vl);
}

void hl::LogError(hl::Color color, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(std::string("ERROR: ") + str.str(), false, color);

    va_end(vl);
}

void hl::LogRaw(hl::Color color, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(str.str(), true, color);

    va_end(vl);
}

#ifdef WIN32
std::string hl::ErrorString(int systemCode)
{
    DWORD code = static_cast<DWORD>(systemCode);
    LPVOID buf = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                   code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
    std::string out = (char*)buf;
    LocalFree(buf);
    return out;
}
#else
std::string hl::ErrorString(int systemCode)
{
    std::string out = std::strerror(systemCode);
    return out;
}
#endif
