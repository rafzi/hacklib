#include "hacklib/Logging.h"
#include "hacklib/Main.h"
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <fstream>


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

static void LogString(const std::string& str, bool raw)
{
    const auto& cfg = getGlobalConfig();

    std::string logStr = str;
    if (!raw && cfg.logTime)
    {
        logStr = GetTimeStr() + " " + str;
    }

    if (cfg.logFunc)
    {
        cfg.logFunc(logStr);
    }

    if (cfg.logToFile)
    {
        std::ofstream logfile(cfg.fileName, std::ios::out | std::ios::app);
        logfile << logStr;
    }
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


void hl::LogDebug(const char* file, const char* func, int line, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(GetCodeStr(file, func, line) + " " + str.str(), false);

    va_end(vl);
}

void hl::LogError(const char* file, const char* func, int line, const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(GetCodeStr(file, func, line) + " ERROR: " + str.str(), false);

    va_end(vl);
}

void hl::LogError(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(std::string("ERROR: ") + str.str(), false);

    va_end(vl);
}

void hl::LogRaw(const char* format, ...)
{
    va_list vl;
    va_start(vl, format);
    const FormatStr str(format, vl);

    LogString(str.str(), true);

    va_end(vl);
}
