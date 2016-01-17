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


hl::StaticInitBase::StaticInitBase()
{
    try
    {
        if (!protectedInit())
        {
            m_unloadPending = true;
        }
    }
    catch (std::exception& e)
    {
        hl::MsgBox("StaticInit error", std::string("Could not perform static initialization: ") + e.what());
        m_unloadPending = true;
    }
    catch (...)
    {
        hl::MsgBox("StaticInit error", "Could not perform static initialization: UNKNOWN EXCEPTION");
        m_unloadPending = true;
    }

    try
    {
        runMainThread();
    }
    catch (std::exception& e)
    {
        hl::MsgBox("StaticInit error", std::string("Could not start thread: ") + e.what());
    }
    catch (...)
    {
        hl::MsgBox("StaticInit error", "Could not start thread: UNKNOWN EXCEPTION");
    }
}

void hl::StaticInitBase::mainThread()
{
    if (m_unloadPending)
    {
        unloadSelf();
    }

    try
    {
        auto pMain = createMainObj();
        m_pMain = pMain.get();

        if (m_pMain->init())
        {
            while (m_pMain->step()) {}
        }
        m_pMain->shutdown();
        m_pMain = nullptr;
    }
    catch (std::exception& e)
    {
        hl::MsgBox("Main Error", std::string("Unhandled C++ exception: ") + e.what());
    }
    catch (...)
    {
        hl::MsgBox("Main Error", "Unhandled C++ exception");
    }

    unloadSelf();
}
