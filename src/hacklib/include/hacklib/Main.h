#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#ifdef _WIN32
#include <Windows.h>

#include "hacklib/MessageBox.h"
#endif


namespace hl
{
    /*
     * This class can be used to start and stop a dynamic library inside a foreign process.
     */
    class Main
    {
    public:
        // Is called on initialization. If returning false, the dll will detach.
        // The default implementation just returns true.
        virtual bool init();
        // Is called continuously sequenially while running. If returning false, the dll will detach.
        // The default implementation sleeps for 10 milliseconds and returns true.
        virtual bool step();
        // Is called on shutdown.
        // The default implementation does nothing.
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
            if (hThread == NULL)
            {
                hl::MsgBox("StaticInit Error", "CreateThread failed with code " + std::to_string(GetLastError()));
            }
            else
            {
                CloseHandle(hThread);
            }
        }

        T *getMain()
        {
            return m_pMain;
        }
        const T *getMain() const
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
                hl::MsgBox("Main Error", "Unhandled C++ exception on Main construction");
            }

            FreeLibraryAndExitThread(hModule, 0);
        }

        static void UserCode(T& main)
        {
            __try
            {
                [&]{
                    try
                    {
                        if (main.init())
                        {
                            while (main.step()) { }
                        }
                        main.shutdown();
                    }
                    catch (...)
                    {
                        hl::MsgBox("Main Error", "Unhandled C++ exception");
                    }
                }();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                []{
                    hl::MsgBox("Main Error", "Unhandled SEH exception");
                }();
            }
        }

    private:
        T *m_pMain = nullptr;

    };

#else

    void *GetCurrentModule();


#endif
}

#endif