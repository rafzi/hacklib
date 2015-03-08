#include "hacklib/Main.h"
#include <thread>
#include <chrono>


#ifdef _WIN32
HMODULE hl::GetCurrentModule()
{
    static HMODULE hModule = NULL;

    if (!hModule)
    {
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCTSTR)GetCurrentModule,
            &hModule);
    }

    return hModule;
}
#else
#include <dlfcn.h>
void *hl::GetCurrentModule()
{
    static void *hModule = nullptr;

    if (!hModule)
    {
        Dl_info info = {0};
        dladdr((void*)GetCurrentModule, &info);
        hModule = info.dli_fbase;
    }

    return hModule;
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