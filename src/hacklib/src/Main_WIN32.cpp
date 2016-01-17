#include "hacklib/Main.h"
#include "hacklib/MessageBox.h"
#include <Windows.h>


hl::ModuleHandle hl::GetCurrentModule()
{
    static hl::ModuleHandle hModule = NULL;

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


static DWORD WINAPI ThreadFunc(LPVOID param)
{
    auto self = (hl::StaticInitBase*)param;

    __try
    {
        self->mainThread();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        char buf[16];
        sprintf(buf, "0x%08X", GetExceptionCode());
        [&]{
            hl::MsgBox("Main Error", std::string("Unhandled SEH exception: ") + buf);
        }();
        self->unloadSelf();
    }
    return 0;
}

void hl::StaticInitBase::runMainThread()
{
    // Must use WinAPI threads instead of std threads, because current std
    // implementation blocks within the loader lock.

    HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, NULL);
    if (hThread == NULL)
    {
        throw std::runtime_error(std::string("CreateThread failed with code ") + std::to_string(GetLastError()));
    }
    else
    {
        // Thread will be exited by suiciding with FreeLibraryAndExitThread.
        CloseHandle(hThread);
    }
}

bool hl::StaticInitBase::protectedInit()
{
    __try
    {
        return init();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        char buf[16];
        sprintf(buf, "0x%08X", GetExceptionCode());
        [&]{
            hl::MsgBox("StaticInit Error", std::string("Unhandled SEH exception: ") + buf);
        }();
    }

    return false;
}

void hl::StaticInitBase::unloadSelf()
{
    FreeLibraryAndExitThread(hl::GetCurrentModule(), 0);
}
