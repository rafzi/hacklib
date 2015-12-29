#include "hacklib/Main.h"
#include "hacklib/WindowOverlay.h"
#include <chrono>
#include <cstdio>

#include <GL/gl.h>


hl::StaticInit<class ExampleMain> g_main;


class ExampleMain : public hl::Main
{
public:
    bool init() override
    {
        printf("hl::Main::init\n");

        m_started = std::chrono::system_clock::now();

        if (m_overlay.create() != hl::WindowOverlay::Error::Okay)
        {
            printf("could not create overlay\n");
        }

        return true;
    }

    bool step() override
    {
        //printf("hl::Main::step\n");

        if (std::chrono::system_clock::now() - m_started > std::chrono::seconds(10))
            return false;

        m_overlay.beginDraw();

        glLineWidth(3.0);
        glColor3f(1.0, 0, 0);
        glBegin(GL_LINES);
        glVertex2f(100, 100);
        glVertex2f(200, 200);
        glEnd();

        m_overlay.swapBuffers();

        return true;
    }

    void shutdown() override
    {
        printf("hl::Main::shutdown\n");
    }

private:
    std::chrono::system_clock::time_point m_started;
    hl::WindowOverlay m_overlay;

};
