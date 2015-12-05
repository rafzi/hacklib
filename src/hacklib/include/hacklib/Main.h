#ifndef HACKLIB_MAIN_H
#define HACKLIB_MAIN_H

#ifdef _WIN32
#include <Windows.h>
#else
#include "hacklib/PageAllocator.h"
#include <dlfcn.h>
#include <pthread.h>
#include <cstring>
#include <thread>

void FreeLibAndExitThread(void *hModule, int(*adr_dlclose)(void*), void(*adr_pthread_exit)(void*));
void FreeLibAndExitThread_after();
#endif


#include "hacklib/MessageBox.h"
#include <string>


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

    std::string GetModulePath();

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
                    catch (std::exception& e)
                    {
                        hl::MsgBox("Main Error", std::string("Unhandled C++ exception: ") + e.what());
                    }
                    catch (...)
                    {
                        hl::MsgBox("Main Error", "Unhandled C++ exception");
                    }
                }();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                char buf[16];
                sprintf(buf, "0x%08X", GetExceptionCode());
                [&]{
                    hl::MsgBox("Main Error", std::string("Unhandled SEH exception: ") + buf);
                }();
            }
        }

    private:
        T *m_pMain = nullptr;

    };

#else

    void *GetCurrentModule();

    template <typename T>
    class StaticInit
    {
    public:
        StaticInit()
        {
            try
            {
                std::thread th(&StaticInit<T>::threadFunc, this);
                th.detach();
            }
            catch (std::exception& e)
            {
                hl::MsgBox("StaticInit error", std::string("Could not start thread: ") + e.what());
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
        void threadFunc()
        {
            try
            {
                T main;
                m_pMain = &main;
                userCode(main);
                m_pMain = nullptr;
            }
            catch (...)
            {
                hl::MsgBox("Main Error", "Unhandled C++ exception on Main construction");
            }

            // Get own module handle by path name. The dlclose just restores the refcount.
            auto modName = hl::GetModulePath();
            auto hModule = dlopen(modName.c_str(), RTLD_LAZY);
            dlclose(hModule);

            /*
            Linux does not have an equivalent to FreeLibraryAndExitThread, so a race between
            dlclose and pthread_exit would exist and lead to crashes.

            The following method of detaching leaks a page = 4KiB each time, but is 100% reliable.
            Alternative: Start a thread on dlclose. => Too unreliable, even with priority adjustments.
            Alternative: Let the injector do dlclose. => Signaling mechanism needed; injector might die or be killed.
            */

            size_t codeSize = (size_t)((uintptr_t)&FreeLibAndExitThread_after - (uintptr_t)&FreeLibAndExitThread);
            hl::code_page_vector code(codeSize);
            memcpy(code.data(), (void*)&FreeLibAndExitThread, codeSize);
            decltype(&FreeLibAndExitThread)(code.data())(hModule, &dlclose, &pthread_exit);
        }

        void userCode(T& main)
        {
            // TODO: sigsegv handler
            try
            {
                if (main.init())
                {
                    while (main.step()) { }
                }
                main.shutdown();
            }
            catch (std::exception& e)
            {
                hl::MsgBox("Main Error", std::string("Unhandled C++ exception: ") + e.what());
            }
            catch (...)
            {
                hl::MsgBox("Main Error", "Unhandled C++ exception");
            }
        }

    private:
        T *m_pMain = nullptr;

    };

#endif
}

#endif
