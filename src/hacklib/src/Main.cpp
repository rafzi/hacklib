#include "hacklib/Main.h"
#include "hacklib/MessageBox.h"
#include "hacklib/CrashHandler.h"
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


static void ProtectedCode(const std::string& location, const std::function<void()>& body)
{
    auto errorStr = "Hacklib error: " + location;

    try
    {
        hl::CrashHandler(body, [&](uint32_t code){
            char buf[16];
#ifdef WIN32
            sprintf(buf, "SEH exception 0x%08X", code);
#else
            sprintf(buf, "signal %i", code);
#endif
            hl::MsgBox(errorStr, buf);
        });
    }
    catch (std::exception& e)
    {
        hl::MsgBox(errorStr, std::string("C++ exception: ") + e.what());
    }
    catch (...)
    {
        hl::MsgBox(errorStr, "Unknown C++ exception");
    }
}


hl::StaticInitImpl::StaticInitImpl()
{
    ProtectedCode("hl::StaticInit construction", [&]{
        runMainThread();
    });
}

void hl::StaticInitImpl::mainThread()
{
    {
        std::unique_ptr<hl::Main> pMain;

        ProtectedCode("hl::Main construction", [&]{
            pMain = makeMain();
        });

        if (pMain)
        {
            m_pMain = pMain.get();

            bool initSuccess = false;
            ProtectedCode("hl::Main::init", [&]{
                initSuccess = m_pMain->init();
            });

            if (initSuccess)
            {
                ProtectedCode("hl::Main::step", [&]{
                    while (m_pMain->step()) { }
                });
            }

            ProtectedCode("hl::Main::shutdown", [&]{
                m_pMain->shutdown();
            });

            m_pMain = nullptr;
        }
    }

    unloadSelf();
}
