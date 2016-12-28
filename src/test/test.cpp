#include "hacklib/Logging.h"
#include "hacklib/CrashHandler.h"
#include "hacklib/Injector.h"
#include "hacklib/Main.h"
#include "hacklib/Memory.h"
#include "hacklib/Patch.h"
#include "hacklib/PatternScanner.h"
#include "hacklib/Hooker.h"
#include "hacklib/PageAllocator.h"
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

void ExecFunc() {}
void ExecFunc_after() {}
void TestMemory()
{
    auto mem = hl::PageAlloc(1000, hl::PROTECTION_READ_WRITE);

    std::copy((char*)ExecFunc, (char*)ExecFunc_after, (char*)mem);

    auto readFunc = [&]{
        bool crashed = false;
        hl::CrashHandler([&]{
            *(volatile int*)mem;
        }, [&](uint32_t){
            crashed = true;
        });
        return crashed;
    };
    auto writeFunc = [&]{
        bool crashed = false;
        hl::CrashHandler([&]{
            *((volatile int*)mem + 100) = 0;
        }, [&](uint32_t){
            crashed = true;
        });
        return crashed;
    };
    auto execFunc = [&]{
        bool crashed = false;
        hl::CrashHandler([&]{
            ((void(*)())mem)();
        }, [&](uint32_t){
            crashed = true;
        });
        return crashed;
    };

    hl::PageProtect(mem, 1000, hl::PROTECTION_NOACCESS);

    HL_ASSERT(readFunc(), "Could read no-access page");
    HL_ASSERT(writeFunc(), "Could write no-access page");
    HL_ASSERT(execFunc(), "Could execute no-access page");

    hl::PageProtect(mem, 1000, hl::PROTECTION_READ);

    HL_ASSERT(!readFunc(), "Could not read read-only page");
    HL_ASSERT(writeFunc(), "Could write read-only page");
    HL_ASSERT(execFunc(), "Could execute read-only page");

    hl::PageProtect(mem, 1000, hl::PROTECTION_READ_WRITE);

    HL_ASSERT(!readFunc(), "Could not read readwrite page");
    HL_ASSERT(!writeFunc(), "Could not write readwrite page");
    HL_ASSERT(execFunc(), "Could execute readwrite page");

    hl::PageProtect(mem, 1000, hl::PROTECTION_READ_EXECUTE);

    HL_ASSERT(!readFunc(), "Could not read executable page");
    HL_ASSERT(writeFunc(), "Could write executable page");
    HL_ASSERT(!execFunc(), "Could not execute executable page");

    hl::PageProtect(mem, 1000, hl::PROTECTION_READ_WRITE_EXECUTE);

    HL_ASSERT(!readFunc(), "Could not read RWE page");
    HL_ASSERT(!writeFunc(), "Could not write RWE page");
    HL_ASSERT(!execFunc(), "Could not execute RWE page");

    hl::PageFree(mem, 1000);
}

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

void TestModules()
{
    auto modPath = hl::GetCurrentModulePath();
    HL_ASSERT(modPath.find("hl_test_host") != std::string::npos, "Wrong module path");

    auto hModule = hl::GetCurrentModule();
    uintptr_t ownFuncAdr = (uintptr_t)&TestModules;
    HL_ASSERT(ownFuncAdr > (uintptr_t)hModule && ownFuncAdr - (uintptr_t)hModule < 0x100000, "Module base address is wrong");

    auto hModuleByName = hl::GetModuleByName(modPath);
    HL_ASSERT(hModule == hModuleByName, "hl::GetModuleByName does not work");

    auto hModuleByAddress = hl::GetModuleByAddress(ownFuncAdr);
    HL_ASSERT(hModule == hModuleByAddress, "hl::GetModuleByAddress does not work");

    static int dummyData = 0;
    auto hModuleByAddressData = hl::GetModuleByAddress((uintptr_t)&dummyData);
    HL_ASSERT(hModule == hModuleByAddressData, "hl::GetModuleByAddress does not work with data");
}

static void DummyFunc()
{
    // Generate some code to play with.
    static volatile int x;
    x++;
    x--;
    x += 42;
    x *= 3;
}
void TestPatch()
{
    auto testAdr = (uintptr_t)&DummyFunc;
    char testData[] = "\x12\34\x56";
    char backupData[3];

    std::copy((char*)testAdr, ((char*)testAdr)+3, backupData);

    {
        hl::Patch patch;
        patch.apply(testAdr, testData, 3);

        HL_ASSERT(std::equal(testData, testData+3, (char*)testAdr), "Patch not applied");
    }

    HL_ASSERT(std::equal(backupData, backupData+3, (char*)testAdr), "Patch not undone");
}
void TestPatternScan()
{
    auto testAdr = (uintptr_t)&DummyFunc;

    hl::Patch patch;
    patch.apply(testAdr, "\x12\x34\x56\x78\x9a\xbc\xde\xf0", 8);

    std::string modName = hl::GetCurrentModulePath();
    auto pattern1 = hl::FindPatternMask("\x12\x34\x56\x78\x9a\xbc\xde\xf0", "xxxxxxxx", testAdr, 0x100);
    auto pattern2 = hl::FindPatternMask("\x12\x34\x56\x78\x9a\xbc\xde\xf0", "xxxxxxxx", modName);
    auto pattern3 = hl::FindPatternMask("\x12\x34\x56\x78\x9a\xbc\xde\xf0", "xxx?xxxx", modName);
    auto pattern4 = hl::FindPattern("12 34 56 78 9a bc de f0", modName);
    auto pattern5 = hl::FindPattern("12 34 56 ?? 9a bc de f0", modName);

    HL_ASSERT(pattern1 == testAdr, "Mask without module info");
    HL_ASSERT(pattern2 == testAdr, "Mask");
    HL_ASSERT(pattern3 == testAdr, "Mask with wildcard");
    HL_ASSERT(pattern4 == testAdr, "String");
    HL_ASSERT(pattern5 == testAdr, "String with wildcard");
}

static int cbCounter = 0;
static void CallbackFunc()
{
    cbCounter++;
}
static void DetourFunc(hl::CpuContext *ctx)
{
    cbCounter++;
}
void TestHooks()
{
    // Set up dummy code.
#ifdef ARCH_64BIT
    hl::code_page_vector code {
        0x55,                   // PUSH RBP
        0x48,0x89,0xe5,         // MOV RSP, RBP
        0x48,0x31,0xc0,         // XOR RAX, RAX
        0x48,0xff,0xc0,         // INC RAX
        0x48,0xff,0xc0,         // INC RAX
        0x48,0xff,0xc0,         // INC RAX
        0x48,0xff,0xc0,         // INC RAX
        0x48,0xff,0xc0,         // INC RAX
        0x5d,                   // POP RBP
        0xc3,                   // RET
    };
    int nextInstrOffset = 16;
#else
    hl::code_page_vector code {
        0x55,                   // PUSH EBP
        0x89,0xec,              // MOV ESP, EBP
        0x31,0xc0,              // XOR EAX, EAX
        0x40,                   // INC EAX
        0x40,                   // INC EAX
        0x40,                   // INC EAX
        0x40,                   // INC EAX
        0x40,                   // INC EAX
        0x5d,                   // POP EBP
        0xc3,                   // RET
    };
    int nextInstrOffset = 6;
#endif

    auto dummyFunc = (int(*)())code.data();

    hl::Hooker hooker;

    uintptr_t jmpBack = 0;
    auto jmpHook = hooker.hookJMP(code.data(), nextInstrOffset, &CallbackFunc, &jmpBack);

    int result = 0;

    cbCounter = 0;
    result = dummyFunc();
    HL_ASSERT(cbCounter == 1, "JMP hook had no effect");

    hooker.unhook(jmpHook);

    cbCounter = 0;
    result = dummyFunc();
    HL_ASSERT(cbCounter == 0, "Hook not undone");
    HL_ASSERT(result == 5, "JMP unhook broke the function");

    auto detourHook = hooker.hookDetour(code.data(), nextInstrOffset, &DetourFunc);

    cbCounter = 0;
    result = dummyFunc();
    HL_ASSERT(cbCounter == 1, "Detour hook had no effect");
    HL_ASSERT(result == 5, "Detour hook broke the function");

    hooker.unhook(detourHook);

    cbCounter = 0;
    result = dummyFunc();
    HL_ASSERT(cbCounter == 0, "Hook not undone");
    HL_ASSERT(result == 5, "Detour unhook broke the function");


    auto memFunc = hl::PageAlloc(1000, hl::PROTECTION_READ_WRITE_EXECUTE);
    auto memVt = hl::PageAlloc(1000, hl::PROTECTION_READ_WRITE_EXECUTE);
    auto memInstance = hl::PageAlloc(1000, hl::PROTECTION_READ_WRITE);
    std::copy((char*)ExecFunc, (char*)ExecFunc_after, (char*)memFunc);
    *(uintptr_t*)memVt = (uintptr_t)memFunc;
    *(uintptr_t*)memInstance = (uintptr_t)memVt;
    hl::PageProtect(memFunc, 1000, hl::PROTECTION_READ_EXECUTE);
    hl::PageProtect(memVt, 1000, hl::PROTECTION_READ);

    auto vtHook = hooker.hookVT(memInstance, 0, &CallbackFunc);

    HL_ASSERT(*(uintptr_t*)memInstance != (uintptr_t)memVt, "Virtual table pointer was not replaced");
    HL_ASSERT(**(uintptr_t**)memInstance == (uintptr_t)&CallbackFunc, "Virtual function was not replaced");

    cbCounter = 0;
    ((void(*)())**(uintptr_t**)memInstance)();
    HL_ASSERT(cbCounter == 1, "Hook had no effect");

    hooker.unhook(vtHook);

    HL_ASSERT(*(uintptr_t*)memInstance == (uintptr_t)memVt, "Virtual table pointer was not restored");

    cbCounter = 0;
    ((void(*)())**(uintptr_t**)memInstance)();
    HL_ASSERT(cbCounter == 0, "Hook not undone");
}


class TestMain : public hl::Main
{
public:
    bool init() override
    {
        // Test requirements for "HL_TEST".
        SetupLogging();
        TestCrashHandler();

        HL_TEST(TestMemory);
        HL_TEST(TestInject);
        HL_TEST(TestModules);
        HL_TEST(TestPatch);
        HL_TEST(TestPatternScan);
        HL_TEST(TestHooks);

        HL_LOG_RAW("==========\nTests finished successfully.\n");
        return false;
    }
};

hl::StaticInit<TestMain> g_main;
