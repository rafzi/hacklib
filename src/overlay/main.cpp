#include "hacklib/WindowOverlay.h"
#include <GL/gl.h>
#include <cstdio>
#include <iostream>

#if WIN32
#include "hacklib/DrawerD3D.h"
#else
#include "hacklib/DrawerOpenGL.h"
#endif

int main()
{
    bool running = true;
    hl::GfxOverlay overlay;
    if (overlay.create(200, 200, 800, 600) == hl::GfxOverlay::Error::Okay)
    // hl::WindowOverlay overlay;
    // if (overlay.create() == hl::GfxOverlay::Error::Okay)
    {
        printf("okay\n");
    }
    else
    {
        printf("not okay\n");
        return 1;
    }

#if WIN32
    hl::DrawerD3D drawer;
#else
    hl::DrawerOpenGL drawer;
#endif
    drawer.setContext(overlay.getContext());

    auto renderLoop = [&]
    {
        while (running)
        {
            overlay.beginDraw();

            drawer.update(hl::Mat4x4(), hl::Mat4x4());

            glViewport(0, 0, overlay.getWidth(), overlay.getHeight());

            // orthographic projection
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, overlay.getWidth(), overlay.getHeight(), 0, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glPushMatrix();
            glTranslatef(50, 50, 0);
            glBegin(GL_QUADS);
            glColor4f(1, 0, 0, 0.5);
            glVertex2f(0, 0);
            glVertex2f(0, 100);
            glVertex2f(200, 100);
            glVertex2f(200, 0);
            glEnd();
            glPopMatrix();

            glPushMatrix();
            glTranslatef(80, 80, 0);
            glBegin(GL_QUADS);
            glColor4f(0, 1, 0, 0.5);
            glVertex2f(0, 0);
            glVertex2f(0, 100);
            glVertex2f(200, 100);
            glVertex2f(200, 0);
            glEnd();
            glPopMatrix();

            drawer.drawLine(50, 50, 200, 100, hl::Color(255, 255, 255));
            drawer.drawRectFilled(50, 50, 300, 50, hl::Color(100, 0, 0, 200));
            drawer.drawRect(60, 60, 280, 30, hl::Color(100, 255, 0, 0));
            drawer.drawCircle(80, 80, 30, hl::Color(150, 0, 255, 0));
            drawer.drawCircleFilled(90, 90, 30, hl::Color(150, 0, 255, 255));

            overlay.swapBuffers();
        }
    };

    std::thread th(renderLoop);
    std::cin.ignore();
    running = false;
    th.join();
    overlay.close();
}
