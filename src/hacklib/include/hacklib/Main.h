#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#ifdef _WIN32
#include <Windows.h>

#include "hacklib/MessageBox.h"
#endif


namespace hl
{
    class Main
    {
    public:
        //Main();

        // TODO copy good doc from hackproj
        // is called on initialization. if returning false, the dll will detach
        virtual bool init();
        // is called continuously sequenially while running. if returning false, the dll will detach
        virtual bool step();
        // is called on shutdown
        virtual void shutdown();

    };

#ifdef _WIN32

    template <typename T>
    class StaticInit
    {
    public:
        StaticInit()
        {
            HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, NULL);
            CloseHandle(hThread);
        }

        T *getMain()
        {
            return m_pMain;
        }

    private:
        static DWORD WINAPI ThreadFunc(LPVOID param)
        {
            HMODULE hModule = GetCurrentModule();
            StaticInit *self = (StaticInit*)param;

            try
            {
                T main;
                self->m_pMain = &main;
                UserCode(main);
                self->m_pMain = nullptr;
            }
            catch (...)
            {
                hl::MsgBox("Main Error", "Unhandled C++ exception");
            }

            FreeLibraryAndExitThread(hModule, 0);
        }

        static void UserCode(T& main)
        {
            __try {
                if (main.init())
                {
                    while (main.step()) { }
                }
                main.shutdown();
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                []{
                    hl::MsgBox("Main Error", "Unhandled SEH exception");
                }();
            }
        }

        static HMODULE GetCurrentModule()
        {
            HMODULE hModule = NULL;
            GetModuleHandleEx(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCTSTR)GetCurrentModule,
                &hModule);

            return hModule;
        }

    private:
        T *m_pMain = nullptr;

    };

#endif
}

#endif