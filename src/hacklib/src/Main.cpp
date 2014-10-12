#include "hacklib/Main.h"
#include "hacklib/MessageBox.h"
#include <thread>
#include <chrono>


//class MainImpl
//{
//public:
//    MainImpl(hl::Main *pMain) : m_pMain(pMain)
//    {
//    }
//
//    void run()
//    {
//        try
//        {
//            if (m_pMain->init())
//            {
//                while (m_pMain->step()) { }
//            } else {
//                hl::MsgBox("Main Error", "Initialization function returned false");
//            }
//
//            m_pMain->shutdown();
//        }
//        catch (...)
//        {
//            hl::MsgBox("Main Error", "Unhandled exception");
//        }
//    }
//
//private:
//    hl::Main * const m_pMain;
//
//};
//
//
//MainImpl *g_mainimpl;


#ifdef _WIN32

#include <Windows.h>


//static HMODULE GetCurrentModule()
//{
//    HMODULE hModule = NULL;
//    GetModuleHandleEx(
//        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
//        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
//        (LPCTSTR)GetCurrentModule,
//        &hModule);
//
//    return hModule;
//}
//
//
//static DWORD WINAPI MainThread(LPVOID param)
//{
//    HMODULE hModule = GetCurrentModule();
//
//    //g_mainimpl = new MainImpl((hl::Main*)param);
//    //g_mainimpl->run();
//    //delete g_mainimpl;
//
//    FreeLibraryAndExitThread(hModule, 0);
//}

//hl::Main::Main()
//{
//    HANDLE hThread = CreateThread(NULL, 0, MainThread, (LPVOID)this, 0, NULL);
//    CloseHandle(hThread);
//}

#endif

#ifdef __unix

    #error TODO

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