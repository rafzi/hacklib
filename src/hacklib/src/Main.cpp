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
    bool success = false;

    // static initialization
    try
    {
        success = protectedInit();
    }
    catch (std::exception& e)
    {
        hl::MsgBox("StaticInit error", std::string("Could not perform static initialization: ") + e.what());
    }
    catch (...)
    {
        hl::MsgBox("StaticInit error", "Could not perform static initialization: UNKNOWN EXCEPTION");
    }

    if (!success)
        unloadSelf();

    // start thread
    success = false;
    try
    {
        runMainThread();
        success = true;
    }
    catch (std::exception& e)
    {
        hl::MsgBox("StaticInit error", std::string("Could not start thread: ") + e.what());
    }
    catch (...)
    {
        hl::MsgBox("StaticInit error", "Could not start thread: UNKNOWN EXCEPTION");
    }

    if (!success)
        unloadSelf();
}

namespace hl
{
    class RAIIHelper
    {
    public:
        RAIIHelper(hl::StaticInitBase* base, std::unique_ptr<hl::Main>&& main)
            : m_base(base)
        {
            m_base->m_pMain = std::move(main);
        }

        ~RAIIHelper()
        {
            m_base->m_pMain.reset();
        }

    private:
        hl::StaticInitBase* const m_base;
    };
}


void hl::StaticInitBase::mainThread()
{
    try
    {
        hl::RAIIHelper guard(this, createMainObj());
        if (m_pMain->init())
        {
            while (m_pMain->step()) {}
        }
        m_pMain->shutdown();
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
