#include "hacklib/GfxOverlay_UNIX.h"
#include "hacklib/WindowOverlay.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stack>
#include <unistd.h>


using namespace hl;


WindowHandle WindowOverlay::GetTargetWindow()
{
    auto display = XOpenDisplay(NULL);
    auto screen = DefaultScreen(display);
    auto root = RootWindow(display, screen);

    auto atomPID = XInternAtom(display, "_NET_WM_PID", True);

    std::stack<Window> windows;
    windows.push(root);

    while (!windows.empty())
    {
        auto hWnd = windows.top();
        windows.pop();

        Atom type = 0;
        int format = 0;
        unsigned long numitems = 0;
        unsigned long bytesafter = 0;
        unsigned char* propPID = nullptr;
        auto result = XGetWindowProperty(display, hWnd, atomPID, 0, 1, False, XA_CARDINAL, &type, &format, &numitems,
                                         &bytesafter, &propPID);
        if (result == Success && propPID != nullptr)
        {
            if (*(pid_t*)propPID == getpid())
            {
                XWindowAttributes attrs = {};
                XGetWindowAttributes(display, hWnd, &attrs);

                if (attrs.width > 0 && attrs.height > 0 && attrs.c_class == InputOutput &&
                    attrs.map_state == IsViewable)
                {
                    XFree(propPID);
                    return hWnd;
                }
            }

            XFree(propPID);
        }

        Window outRoot = 0;
        Window outParent = 0;
        Window* outChildren = nullptr;
        unsigned int numchildren = 0;
        if (XQueryTree(display, hWnd, &outRoot, &outParent, &outChildren, &numchildren))
        {
            for (unsigned int i = 0; i < numchildren; i++)
            {
                windows.push(outChildren[i]);
            }
        }
    }

    XCloseDisplay(display);

    return 0;
}


void WindowOverlay::cbWindowLoop()
{
    auto display = m_impl->display;
    auto root = RootWindow(display, m_impl->screen);

    XWindowChanges changes = {};
    unsigned int changeMask = 0;

    XWindowAttributes attrs = {};
    XGetWindowAttributes(display, m_targetWindow, &attrs);

    int absX = 0, absY = 0;
    Window child = 0;
    XTranslateCoordinates(display, m_targetWindow, root, 0, 0, &absX, &absY, &child);

    const int posX = absX - attrs.x;
    const int posY = absY - attrs.y;
    if (posX != m_posX || posY != m_posY)
    {
        m_posX = changes.x = posX;
        m_posY = changes.y = posY;
        changeMask |= CWX | CWY;
    }

    if (attrs.width != m_width || attrs.height != m_height)
    {
        m_width = changes.width = attrs.width;
        m_height = changes.height = attrs.height;
        changeMask |= CWWidth | CWHeight;
    }

    if (changeMask && m_hWnd)
    {
        XConfigureWindow(display, m_hWnd, changeMask, &changes);
    }
}
