#include "hacklib/Main.h"
#include "hacklib/MessageBox.h"
#include <thread>
#include <chrono>


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