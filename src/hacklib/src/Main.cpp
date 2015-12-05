#include "hacklib/Main.h"
#include "hacklib/MessageBox.h"
#include <thread>
#include <chrono>
#include <stdexcept>


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


hl::StaticInitImpl::StaticInitImpl()
{
    try
    {
        createThread();
    }
    catch (std::exception& e)
    {
        hl::MsgBox("StaticInit error", std::string("Could not start thread: ") + e.what());
    }
}

void hl::StaticInitImpl::mainThread()
{
    try
    {
        constructAndRun();
    }
    catch (std::exception& e)
    {
        hl::MsgBox("Main Error", std::string("Unhandled C++ exception on Main construction: ") + e.what());
    }
    catch (...)
    {
        hl::MsgBox("Main Error", "Unhandled C++ exception on Main construction");
    }

    unloadSelf();
}

void hl::StaticInitImpl::runMain(hl::Main& main)
{
    m_pMain = &main;

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

    m_pMain = nullptr;
}
