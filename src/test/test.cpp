#include "hacklib/Logging.h"
#include "hacklib/CrashHandler.h"
#include "hacklib/Injector.h"
#include "hacklib/Main.h"
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <cstdio>
#include <stdexcept>


#define HL_ASSERT(cond, format, ...) \
    do { \
        if (!(cond)) \
        { \
            HL_LOG_RAW("Assertion '%s' failed.\n  msg: ", #cond); \
            HL_LOG_RAW(format, ##__VA_ARGS__); \
            HL_LOG_RAW("\n  in %s:%i\n  func: %s\n", __FILE__, __LINE__, __FUNCTION__); \
            std::cin.ignore(); \
            exit(1); \
        } \
    } while (false)

#define HL_TEST(testBody) \
    do { \
        hl::CrashHandler([&]{ \
            try \
            { \
                HL_LOG_RAW("Starting test: \"%s\"\n", #testBody); \
                testBody(); \
            } \
            catch (std::exception& e) \
            { \
                HL_ASSERT(false, "Test failed with C++ exception: %s", e.what()); \
            } \
            catch (...) \
            { \
                HL_ASSERT(false, "Test failed with unknown C++ exception"); \
            } \
        }, [](uint32_t code){ \
            HL_ASSERT(false, "Test failed with system exception code %08X", code); \
        }); \
    } while (false)


void SetupLogging(bool silent = false)
{
    if (!silent)
    {
        printf("Testing logging functionality.\n");
    }

    hl::LogConfig logCfg;
    logCfg.logToFile = false;
    logCfg.logTime = false;
    logCfg.logFunc = [](const std::string& msg){ printf("%s", msg.c_str()); };
    hl::ConfigLog(logCfg);

    if (!silent)
    {
        printf("Test log: [Expected: \"Test message\"]\n");
        HL_LOG_RAW("Test message\n");
    }
}

void TestCrashHandler()
{
    hl::CrashHandler([]{ }, [](uint32_t){ HL_ASSERT(false, "CrashHandler handler called without error"); });

    bool handlerCalled = false;
    bool fellThrough = false;
    try
    {
        hl::CrashHandler([]{ throw int(); }, [&](uint32_t){ handlerCalled = true; });
    }
    catch (...)
    {
        fellThrough = true;
    }
    HL_ASSERT(!handlerCalled, "CrashHandler called on C++ exception");
    HL_ASSERT(fellThrough, "CrashHandler did not let C++ exception fall through");

    handlerCalled = false;
    hl::CrashHandler([]{ int crash = *(volatile int*)nullptr; }, [&](uint32_t){ handlerCalled = true; });
    HL_ASSERT(handlerCalled, "CrashHandler not called on system exception");
}

// TODO add to hacklib?
class Process
{
public:
    Process(int id, uintptr_t handle) : m_id(id), m_handle(handle) { }
    Process(const Process&) = delete;
    Process(Process&& other) { m_id = other.m_id; m_handle = other.m_handle; } // = default
    int id() const { return m_id; }
    int join();
private:
    int m_id;
    uintptr_t m_handle;
};
Process LaunchProcess(const std::string& command, const std::vector<std::string>& args = { });
#ifdef WIN32
#include <Windows.h>
int Process::join()
{
    if (!m_id)
    {
        throw std::runtime_error("Process is not joinable");
    }
    if (WaitForSingleObject((HANDLE)m_handle, INFINITE) != WAIT_OBJECT_0)
    {
        throw std::runtime_error("WaitForSingleObject failed");
    }
    DWORD exitCode;
    if (GetExitCodeProcess((HANDLE)m_handle, &exitCode) == 0)
    {
        throw std::runtime_error("GetExitCodeProcess failed");
    }
    m_id = 0;
    CloseHandle((HANDLE)m_handle);
    return exitCode;
}
Process LaunchProcess(const std::string& command, const std::vector<std::string>& args)
{
    std::string cmdline = command;
    for (const auto& arg : args)
    {
        cmdline += " ";
        cmdline += arg;
    }

    STARTUPINFOA startupInfo = { };
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo;
    BOOL result = CreateProcessA(
        NULL,
        const_cast<char*>(cmdline.c_str()),
        NULL,
        NULL,
        false,
        0,
        NULL,
        NULL,
        &startupInfo,
        &processInfo);

    if (!result)
    {
        throw std::runtime_error("CreateProcess failed");
    }

    return Process(processInfo.dwProcessId, (uintptr_t)processInfo.hProcess);
}
#else
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
int Process::join()
{
    if (!m_id)
    {
        throw std::runtime_error("Process is not joinable");
    }
    int status, result;
    do
    {
        result = waitpid(m_id, &status, 0);
    } while (result < 0 && errno == EINTR);
    if (result != m_id)
    {
        throw std::runtime_error("waitpid failed");
    }
    m_id = 0;
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return -WTERMSIG(status);
    else
        return -1;
}
Process LaunchProcess(const std::string& command, const std::vector<std::string>& args)
{
    // FIXME only for convenience, but should be removed for security.
    auto currentDirCommand = "./" + command;

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(currentDirCommand.c_str()));
    for (const auto& arg : args)
    {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    int pid = fork();
    if (pid < 0)
    {
        throw std::runtime_error("fork failed");
    }
    else if (pid == 0)
    {
        execvp(argv[0], &argv[0]);
        // Will only reach here on error.
        // Call special exit to prevent parent process doing double cleanup.
        _exit(72);
    }

    return Process(pid, 0);
}
#endif

void TestInject()
{
    std::string libName = "hl_test_lib";
#ifdef _DEBUG
    libName += "d";
#endif
#ifdef WIN32
    libName += ".dll";
#else
    libName = "lib" + libName + ".so";
#endif

    auto process = LaunchProcess("hl_test", { "--child" });

    // Give the child time to set everything up.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::string error;
    auto result = hl::Inject(process.id(), libName, &error);
    HL_LOG_RAW("hl::Inject error string:\n%s\n", error.c_str());
    HL_ASSERT(result, "Injection failed");

    error = "";
    result = !hl::Inject(process.id(), libName, &error);
    HL_LOG_RAW("hl::Inject error string:\n%s\n", error.c_str());
    HL_ASSERT(result, "Double Injection must fail");

    // Give the library time to detach itself.
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    error = "";
    result = hl::Inject(process.id(), libName, &error);
    HL_LOG_RAW("hl::Inject error string:\n%s\n", error.c_str());
    HL_ASSERT(result, "Re-Injection must succeed");

    HL_ASSERT(process.join() == 0, "");
}


class TestMain : public hl::Main
{
public:
    bool init() override
    {
        // Test requirements for "HL_TEST".
        SetupLogging();
        TestCrashHandler();

        HL_TEST(TestInject);

        HL_LOG_RAW("==========\nTests finished successfully.\n");
        return false;
    }
};

hl::StaticInit<TestMain> g_main;
