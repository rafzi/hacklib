#include "hacklib/CrashHandler.h"
#include "hacklib/Hooker.h"
#include "hacklib/ImplementMember.h"
#include "hacklib/Injector.h"
#include "hacklib/Logging.h"
#include "hacklib/Main.h"
#include "hacklib/Memory.h"
#include "hacklib/PageAllocator.h"
#include "hacklib/Patch.h"
#include "hacklib/PatternScanner.h"
#include <chrono>
#include <cstdio>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>


#define HL_ASSERT(cond, format, ...)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            HL_LOG_RAW("Assertion '%s' failed.\n  msg: ", #cond);                                                      \
            HL_LOG_RAW(format, ##__VA_ARGS__);                                                                         \
            HL_LOG_RAW("\n  in %s:%i\n  func: %s\n", __FILE__, __LINE__, __FUNCTION__);                                \
            std::cin.ignore();                                                                                         \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (false)

#define HL_TEST(testBody)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        hl::CrashHandler(                                                                                              \
            [&]                                                                                                        \
            {                                                                                                          \
                try                                                                                                    \
                {                                                                                                      \
                    HL_LOG_RAW("Starting test: \"%s\"\n", #testBody);                                                  \
                    testBody();                                                                                        \
                }                                                                                                      \
                catch (std::exception & e)                                                                             \
                {                                                                                                      \
                    HL_ASSERT(false, "Test failed with C++ exception: %s", e.what());                                  \
                }                                                                                                      \
                catch (...)                                                                                            \
                {                                                                                                      \
                    HL_ASSERT(false, "Test failed with unknown C++ exception");                                        \
                }                                                                                                      \
            },                                                                                                         \
            [](uint32_t code) { HL_ASSERT(false, "Test failed with system exception code %08X", code); });             \
    } while (false)


static void SetupLogging(bool silent = false)
{
    if (!silent)
    {
        printf("Testing logging functionality.\n");
    }

    hl::LogConfig logCfg;
    logCfg.logToFile = false;
    logCfg.logTime = false;
    logCfg.logFunc = [](const std::string& msg) { printf("%s", msg.c_str()); };
    hl::ConfigLog(logCfg);

    if (!silent)
    {
        printf("Test log: [Expected: \"Test message\"]\n");
        HL_LOG_RAW("Test message\n");
    }
}

static void TestCrashHandler()
{
    hl::CrashHandler([] {}, [](uint32_t) { HL_ASSERT(false, "CrashHandler handler called without error"); });

    bool handlerCalled = false;
    bool fellThrough = false;
    try
    {
        hl::CrashHandler([] { throw int(); }, [&](uint32_t) { handlerCalled = true; });
    }
    catch (...)
    {
        fellThrough = true;
    }
    HL_ASSERT(!handlerCalled, "CrashHandler called on C++ exception");
    HL_ASSERT(fellThrough, "CrashHandler did not let C++ exception fall through");

    handlerCalled = false;
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    hl::CrashHandler([] { (void)*(volatile int*)nullptr; }, [&](uint32_t) { handlerCalled = true; });
    HL_ASSERT(handlerCalled, "CrashHandler not called on system exception");
}

// TODO add to hacklib?
class Process
{
public:
    Process(int id, uintptr_t handle) : m_id(id), m_handle(handle) {}
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&& other) = default;
    Process& operator=(Process&& other) = default;
    ~Process()
    {
        if (m_id)
        {
            try
            {
                join();
            }
            catch (std::runtime_error& e)
            {
                HL_LOG_ERR("Error joining process: %s\n", e.what());
            }
        }
    }
    [[nodiscard]] int id() const { return m_id; }
    int join();

private:
    int m_id;
    [[maybe_unused]] uintptr_t m_handle;
};
static Process LaunchProcess(const std::string& command, std::vector<std::string> args = {});
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
Process LaunchProcess(const std::string& command, std::vector<std::string> args)
{
    std::string cmdline = command;
    for (const auto& arg : std::move(args))
    {
        cmdline += " ";
        cmdline += arg;
    }

    STARTUPINFOA startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    BOOL result = CreateProcessA(NULL, const_cast<char*>(cmdline.c_str()), NULL, NULL, false, 0, NULL, NULL,
                                 &startupInfo, &processInfo);

    if (!result)
    {
        throw std::runtime_error("CreateProcess failed");
    }
    CloseHandle(processInfo.hThread);

    return { (int)processInfo.dwProcessId, (uintptr_t)processInfo.hProcess };
}
#else
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
int Process::join()
{
    if (!m_id)
    {
        throw std::runtime_error("Process is not joinable");
    }
    int status = 0, result = 0;
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
    if (WIFSIGNALED(status))
        return -WTERMSIG(status);
    return -1;
}
Process LaunchProcess(const std::string& command, std::vector<std::string> args)
{
    // FIXME only for convenience, but should be removed for security.
    auto currentDirCommand = "./" + command;

    std::vector<char*> argv;
    argv.push_back(currentDirCommand.data());
    for (auto& arg : args)
    {
        argv.push_back(arg.data());
    }
    argv.push_back(nullptr);

    const int pid = fork();
    if (pid < 0)
    {
        throw std::runtime_error("fork failed");
    }

    if (pid == 0)
    {
        execvp(argv[0], argv.data());
        // Will only reach here on error.
        // Call special exit to prevent parent process doing double cleanup.
        _exit(72);
    }

    return { pid, 0 };
}
#endif


// Set up dummy code.
#ifdef ARCH_64BIT
static hl::code_page_vector g_dummyCode{
    0x55,             // PUSH RBP
    0x48, 0x89, 0xe5, // MOV RBP, RSP
    0x48, 0x31, 0xc0, // XOR RAX, RAX
    0x48, 0xff, 0xc0, // INC RAX
    0x48, 0xff, 0xc0, // INC RAX
    0x48, 0xff, 0xc0, // INC RAX
    0x48, 0xff, 0xc0, // INC RAX
    0x48, 0xff, 0xc0, // INC RAX
    0x5d,             // POP RBP
    0xc3,             // RET
};
static int g_dummyHookOffset = 16;
#else
static hl::code_page_vector g_dummyCode{
    0x55,       // PUSH EBP
    0x89, 0xe5, // MOV EBP, ESP
    0x31, 0xc0, // XOR EAX, EAX
    0x40,       // INC EAX
    0x40,       // INC EAX
    0x40,       // INC EAX
    0x40,       // INC EAX
    0x40,       // INC EAX
    0x5d,       // POP EBP
    0xc3,       // RET
};
static int g_dummyHookOffset = 6;
#endif

template <typename T, typename F>
static void ExpectException(F func)
{
    bool gotException = false;
    try
    {
        func();
    }
    catch (const T&)
    {
        gotException = true;
    }
    HL_ASSERT(gotException, "Did not get expected exception");
}


static void TestMemory()
{
    void* mem = g_dummyCode.data();

    auto readFunc = [&]
    {
        bool crashed = false;
        hl::CrashHandler(
            [&]
            {
                *(volatile char*)mem;
                *((volatile char*)mem + g_dummyCode.size());
            },
            [&](uint32_t) { crashed = true; });
        return crashed;
    };
    auto backupFirst = g_dummyCode[0];
    auto backupLast = g_dummyCode[g_dummyCode.size() - 1];
    auto writeFunc = [&]()
    {
        bool crashed = false;
        hl::CrashHandler(
            [&]
            {
                *(volatile char*)mem = 0;
                *((volatile char*)mem + g_dummyCode.size()) = 0;
                *(volatile unsigned char*)mem = backupFirst;
                *((volatile unsigned char*)mem + g_dummyCode.size()) = backupLast;
            },
            [&](uint32_t) { crashed = true; });
        return crashed;
    };
    auto execFunc = [&]
    {
        bool crashed = false;
        hl::CrashHandler([&] { ((void (*)())mem)(); }, [&](uint32_t) { crashed = true; });
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

    // Restore normal code protection.
    hl::PageProtect(mem, 1000, hl::PROTECTION_READ_EXECUTE);


    // Test across page boundary.
    hl::data_page_vector<unsigned char> twoPages(2 * hl::GetPageSize());
    auto offset = hl::GetPageSize() - g_dummyCode.size() + 1;
    mem = twoPages.data() + offset;
    std::ranges::copy(g_dummyCode, twoPages.begin() + static_cast<std::ptrdiff_t>(offset));
    hl::PageProtectVec(twoPages, hl::PROTECTION_NOACCESS);
    hl::PageProtect(mem, g_dummyCode.size(), hl::PROTECTION_READ_WRITE_EXECUTE);

    HL_ASSERT(!readFunc(), "Could not read RWE page across page boundary");
    HL_ASSERT(!writeFunc(), "Could not write RWE page across page boundary");
    HL_ASSERT(!execFunc(), "Could not execute RWE page across page boundary");
}

static void TestInject()
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

static void TestModules()
{
    auto modPath = hl::GetCurrentModulePath();
    HL_ASSERT(modPath.find("hl_test_host") != std::string::npos, "Wrong module path");

    auto hModule = hl::GetCurrentModule();
    auto ownFuncAdr = (uintptr_t)&TestModules;
    HL_ASSERT(ownFuncAdr > (uintptr_t)hModule && ownFuncAdr - (uintptr_t)hModule < 0x100000,
              "Module base address is wrong");

    auto hModuleByName = hl::GetModuleByName(modPath);
    HL_ASSERT(hModule == hModuleByName, "hl::GetModuleByName does not work");

    auto hModuleByAddress = hl::GetModuleByAddress(ownFuncAdr);
    HL_ASSERT(hModule == hModuleByAddress, "hl::GetModuleByAddress does not work");

    static int dummyData = 0;
    auto hModuleByAddressData = hl::GetModuleByAddress((uintptr_t)&dummyData);
    HL_ASSERT(hModule == hModuleByAddressData, "hl::GetModuleByAddress does not work with data");
}

static void TestPatch()
{
    auto testAdr = (uintptr_t)g_dummyCode.data();
    char testData[] = "\x12\34\x56";
    char backupData[3];

    std::copy((char*)testAdr, ((char*)testAdr) + 3, backupData);

    {
        hl::Patch patch;
        patch.apply(testAdr, testData, 3);

        HL_ASSERT(std::equal(testData, testData + 3, (char*)testAdr), "Patch not applied");
    }

    HL_ASSERT(std::equal(backupData, backupData + 3, (char*)testAdr), "Patch not undone");

    {
        hl::Patch patch;
        patch.apply(testAdr, (uint16_t)0xabcd);

        HL_ASSERT(*(uint16_t*)testAdr == 0xabcd, "Patch not applied");

        const hl::Patch patch_moved = std::move(patch);
        HL_ASSERT(*(uint16_t*)testAdr == 0xabcd, "Patch undone too early");
    }

    HL_ASSERT(std::equal(backupData, backupData + 3, (char*)testAdr), "Patch not undone");
}

class TestImplMember
{
public:
    IMPLMEMBER(int, Member1, 0x20);
    IMPLVTFUNC_OR(void, SomeFunction, 13, int, argument1);
};

static void TestPatternScan()
{
    auto testAdr = (uintptr_t)g_dummyCode.data();

    hl::Patch patch;
    patch.apply(testAdr, "\x12\x34\x56\x78\x9a\xbc\xde\xf0", 8);

    auto pattern1 = hl::FindPatternMask("\x12\x34\x56\x78\x9a\xbc\xde\xf0", "xxxxxxxx", testAdr, 0x100);
    auto pattern2 = hl::FindPatternMask("\x12\x34\x56\x78\x9a\xbc\xde\xf0", "xxx?xxxx", testAdr, 0x100);
    auto pattern3 = hl::FindPattern("12 34 56 78 9a bc de f0", testAdr, 0x100);
    auto pattern4 = hl::FindPattern("12 34 56 ?? 9a bc de f0", testAdr, 0x100);
    auto pattern5 = hl::FindPattern("12 34 56 78 9a bC dE f0", testAdr, 0x100);

    HL_ASSERT(pattern1 == testAdr, "Mask");
    HL_ASSERT(pattern2 == testAdr, "Mask with wildcard");
    HL_ASSERT(pattern3 == testAdr, "String");
    HL_ASSERT(pattern4 == testAdr, "String with wildcard");
    HL_ASSERT(pattern5 == testAdr, "Uppercase");

    ExpectException<std::runtime_error>([&] { hl::FindPattern("12 34 g0 78", testAdr, 0x100); });
    ExpectException<std::runtime_error>([&] { hl::FindPattern("12 34 ?0 78", testAdr, 0x100); });

    // Surround the search area with noaccess pages to check for out of bounds accesses.
    auto pageSize = hl::GetPageSize();
    hl::code_page_vector guardMem(3 * pageSize);
    hl::PageProtect(guardMem.data(), pageSize, hl::PROTECTION_NOACCESS);
    hl::PageProtect((void*)((uintptr_t)guardMem.data() + 2 * pageSize), pageSize, hl::PROTECTION_NOACCESS);
    hl::Patch baitPatch;
    baitPatch.apply((uintptr_t)guardMem.data() + 2 * pageSize - 4, (uint32_t)0xcccccccc);

    auto patternGuard1 = hl::FindPattern("cc cc cc cc cc", (uintptr_t)guardMem.data() + pageSize, pageSize);
    auto patternGuard2 = hl::FindPattern("cc cc cc cc", (uintptr_t)guardMem.data() + pageSize, pageSize);

    HL_ASSERT(patternGuard1 == 0, "Should not find this");
    HL_ASSERT(patternGuard2 == (uintptr_t)guardMem.data() + 2 * pageSize - 4, "Should find this");
}

static int cbCounter = 0;
static void CallbackFunc()
{
    cbCounter++;
}
static void DetourFunc(hl::CpuContext* ctx)
{
    cbCounter++;
}
static void TestHooks()
{
    auto dummyFunc = (int (*)())g_dummyCode.data();

    hl::Hooker hooker;

    uintptr_t jmpBack = 0;
    auto jmpHook = hooker.hookJMP(g_dummyCode.data(), g_dummyHookOffset, &CallbackFunc, &jmpBack);


    cbCounter = 0;
    dummyFunc();
    HL_ASSERT(cbCounter == 1, "JMP hook had no effect");

    hooker.unhook(jmpHook);

    cbCounter = 0;
    int result = dummyFunc();
    HL_ASSERT(cbCounter == 0, "Hook not undone");
    HL_ASSERT(result == 5, "JMP unhook broke the function");

    auto detourHook = hooker.hookDetour(g_dummyCode.data(), g_dummyHookOffset, &DetourFunc);

    cbCounter = 0;
    result = dummyFunc();
    HL_ASSERT(cbCounter == 1, "Detour hook had no effect");
    HL_ASSERT(result == 5, "Detour hook broke the function");

    hooker.unhook(detourHook);

    cbCounter = 0;
    result = dummyFunc();
    HL_ASSERT(cbCounter == 0, "Hook not undone");
    HL_ASSERT(result == 5, "Detour unhook broke the function");

    auto memVt = hl::PageAlloc(1000, hl::PROTECTION_READ_WRITE_EXECUTE);
    auto memInstance = hl::PageAlloc(1000, hl::PROTECTION_READ_WRITE);
    *(uintptr_t*)memVt = (uintptr_t)g_dummyCode.data();
    *(uintptr_t*)memInstance = (uintptr_t)memVt;
    hl::PageProtect(memVt, 1000, hl::PROTECTION_READ);
    auto vtHook = hooker.hookVT(memInstance, 0, &CallbackFunc);

    HL_ASSERT(*(uintptr_t*)memInstance != (uintptr_t)memVt, "Virtual table pointer was not replaced");
    HL_ASSERT(**(uintptr_t**)memInstance == (uintptr_t)&CallbackFunc, "Virtual function was not replaced");

    cbCounter = 0;
    ((void (*)()) * *(uintptr_t**)memInstance)();
    HL_ASSERT(cbCounter == 1, "Hook had no effect");

    hooker.unhook(vtHook);

    HL_ASSERT(*(uintptr_t*)memInstance == (uintptr_t)memVt, "Virtual table pointer was not restored");

    cbCounter = 0;
    ((void (*)()) * *(uintptr_t**)memInstance)();
    HL_ASSERT(cbCounter == 0, "Hook not undone");
}

#ifdef WIN32
static int vehCounter1 = 0;
static int vehCounter2 = 0;
static void VEHFunc1(hl::CpuContext* ctx)
{
    vehCounter1++;
}
static void VEHFunc2(hl::CpuContext* ctx)
{
    vehCounter2++;
}
void TestVEH()
{
    hl::code_page_vector dummyCode(2 * hl::GetPageSize());
    std::copy(g_dummyCode.begin(), g_dummyCode.end(), dummyCode.begin());
    std::copy(g_dummyCode.begin(), g_dummyCode.end(), dummyCode.begin() + hl::GetPageSize());
    void* funcPage1 = dummyCode.data();
    void* funcPage2 = dummyCode.data() + hl::GetPageSize();

    hl::Hooker hooker;
    auto hook2 = hooker.hookVEH((uintptr_t)funcPage2, VEHFunc2);

    ((void (*)())funcPage2)();
    HL_ASSERT(vehCounter2 == 1, "");

    auto hook1 = hooker.hookVEH((uintptr_t)funcPage1, VEHFunc1);

    ((void (*)())funcPage1)();
    HL_ASSERT(vehCounter1 == 1, "");
    HL_ASSERT(vehCounter2 == 1, "");

    ((void (*)())funcPage2)();
    HL_ASSERT(vehCounter1 == 1, "");
    HL_ASSERT(vehCounter2 == 2, "");

    ((void (*)())funcPage2)();
    HL_ASSERT(vehCounter1 == 1, "");
    HL_ASSERT(vehCounter2 == 3, "");

    hooker.unhook(hook1);

    ((void (*)())funcPage1)();
    ((void (*)())funcPage2)();
    HL_ASSERT(vehCounter1 == 1, "");
    HL_ASSERT(vehCounter2 == 4, "");

    ((void (*)())funcPage1)();
    ((void (*)())funcPage2)();
    HL_ASSERT(vehCounter1 == 1, "");
    HL_ASSERT(vehCounter2 == 5, "");

    hooker.unhook(hook2);

    ((void (*)())funcPage1)();
    ((void (*)())funcPage2)();
    HL_ASSERT(vehCounter1 == 1, "");
    HL_ASSERT(vehCounter2 == 5, "");
}
#else
static void TestVEH() {}
#endif


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
        HL_TEST(TestVEH);

        HL_LOG_RAW("==========\nTests finished successfully.\n");
        return false;
    }
};

static hl::StaticInit<TestMain> g_main;
