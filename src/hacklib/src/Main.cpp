#include "hacklib/Main.h"
#include <thread>
#include <chrono>
#include <stdexcept>


#ifdef _WIN32
HMODULE hl::GetCurrentModule()
{
    static HMODULE hModule = NULL;

    if (!hModule)
    {
        if (GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCTSTR)GetCurrentModule,
            &hModule) == 0)
        {
            throw std::runtime_error("GetModuleHandleEx failed");
        }
    }

    return hModule;
}
std::string hl::GetModulePath()
{
    static std::string modulePath;

    if (modulePath == "")
    {
        char path[MAX_PATH];
        if (GetModuleFileNameA(hl::GetCurrentModule(), path, MAX_PATH) == 0)
        {
            throw std::runtime_error("GetModuleFileName failed");
        }
        modulePath = path;
    }

    return modulePath;
}
#else
#include <dlfcn.h>
void *hl::GetCurrentModule()
{
    static void *hModule = nullptr;

    if (!hModule)
    {
        Dl_info info = {0};
        if (dladdr((void*)GetCurrentModule, &info) == 0)
        {
            throw std::runtime_error("dladdr failed");
        }
        hModule = info.dli_fbase;
    }

    return hModule;
}
std::string hl::GetModulePath()
{
    static std::string modulePath;

    if (modulePath == "")
    {
        Dl_info info = { 0 };
        if (dladdr((void*)GetModulePath, &info) == 0)
        {
            throw std::runtime_error("dladdr failed");
        }
        modulePath = info.dli_fname;
    }

    return modulePath;
}
#endif


bool hl::Main::init()
{
    return true;
}

bool hl::Main::step()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return true;
}

void hl::Main::shutdown()
{

}