#include "hacklib/Main.h"
#include <chrono>
#include <cstdio>


hl::StaticInit<class ExampleMain> g_main;


class ExampleMain : public hl::Main
{
public:
    bool init() override
    {
        printf("hl::Main::init\n");

        m_started = std::chrono::system_clock::now();

        return true;
    }

    bool step() override
    {
        //printf("hl::Main::step\n");

        if (std::chrono::system_clock::now() - m_started > std::chrono::seconds(10))
            return false;

        return true;
    }

    void shutdown() override
    {
        printf("hl::Main::shutdown\n");
    }

private:
    std::chrono::system_clock::time_point m_started;

};
