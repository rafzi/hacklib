#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#ifdef _WIN32
#include <Windows.h>

#include "hacklib/MessageBox.h"
#endif


namespace hl
{
    /*
     * This class can be used to do start and stop a dynamic library inside a foreign process.
     *
     * If you derive from this class and use it for static initialization,
     * make sure that the constructor and destructor are safe for execution
     * while in a loader lock. If initialization or cleanup requires synchronization,
     * override the init and shutdown member functions.
     */
    class Main
    {
    public:
        // Is called on initialization. If returning false, the dll will detach.
        virtual bool init();
        // Is called continuously sequenially while running. If returning false, the dll will detach.
        virtual bool step();
        // Is called on shutdown.
        virtual void shutdown();

    };

#ifdef _WIN32

    HMODULE GetCurrentModule();

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

    private:
        T *m_pMain = nullptr;

    };

#endif
}

#endif