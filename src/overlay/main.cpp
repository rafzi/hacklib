#include "hacklib/WindowOverlay.h"
#include <GL/gl.h>
#include <iostream>
#include <cstdio>


int main()
{
    bool running = true;
    hl::GfxOverlay overlay;
    if (overlay.create(200, 200, 800, 600) == hl::GfxOverlay::Error::Okay)
    //hl::WindowOverlay overlay;
    //if (overlay.create() == hl::GfxOverlay::Error::Okay)
    {
        printf("okay\n");
    }
    else
    {
        printf("not okay\n");
        return 1;
    }

    auto renderLoop = [&]{
        while (running)
        {
            overlay.beginDraw();

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

            overlay.swapBuffers();
        }
    };

    std::thread th(renderLoop);
    std::cin.ignore();
    running = false;
    th.join();
    overlay.close();
}
