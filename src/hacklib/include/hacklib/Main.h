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
                if (main.init())
                {
                    while (main.step()) { }
                } else {
                    hl::MsgBox("Main Error", "Initialization function returned false");
                }

                main.shutdown();
                self->m_pMain = nullptr;
            }
            catch (...)
            {
                hl::MsgBox("Main Error", "Unhandled exception");
            }

            FreeLibraryAndExitThread(hModule, 0);
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