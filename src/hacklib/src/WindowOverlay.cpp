#include "hacklib/WindowOverlay.h"
#include <dwmapi.h>


using namespace hl;


static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    /* criteria: owned by current process, visible, not a child window and client size > 0
        - owned by current process
        - visible
        - not a child window
        - created by main module (exe)
        - client size > 0
    */
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == GetCurrentProcessId()) {
        LONG_PTR styles = GetWindowLongPtr(hWnd, GWL_STYLE);
        HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        if ((styles & WS_VISIBLE) && !(styles & WS_CHILDWINDOW) && hinst == GetModuleHandle(NULL)) {
            RECT rect;
            GetClientRect(hWnd, &rect);
            if (rect.right != 0 && rect.bottom != 0)
            {
                *(HWND*)lParam = hWnd;
            }
        }
    }

    return TRUE;
}



WindowOverlay::WindowOverlay(HINSTANCE hModule) : GfxOverlay(hModule)
{
}


GfxOverlay::Error WindowOverlay::create(bool alwaysOnTop)
{
    m_alwaysOnTop = alwaysOnTop;

    // get window that belongs to current process
    m_targetWindow = NULL;
    if (!EnumWindows(EnumWindowsProc, (LPARAM)&m_targetWindow) || !m_targetWindow)
        return Error::Other;

    // adjust metrics
    cbWindowLoop();
    m_visible = true;

    return GfxOverlay::create(m_posX, m_posY, m_width, m_height);
}


void WindowOverlay::cbWindowLoop()
{
    // this method makes sure that the overlay is always matching the target window

    bool changeFound = false;
    UINT setWndPosFlags = 0;

    // window position changes
    POINT pt = { 0, 0 };
    if (!ClientToScreen(m_targetWindow, &pt) ||
        (m_posX == pt.x && m_posY == pt.y))
    {
        setWndPosFlags |= SWP_NOMOVE;
    } else {
        m_posX = pt.x;
        m_posY = pt.y;
        changeFound = true;
    }

    // window size changes
    RECT rect;
    if (!GetClientRect(m_targetWindow, &rect) ||
        (m_width == rect.right && m_height == rect.bottom))
    {
        setWndPosFlags |= SWP_NOSIZE;
    } else {
        m_width = rect.right;
        m_height = rect.bottom;
        changeFound = true;
    }

    // window visibility changes
    HWND zOrderChange = NULL;
    bool isForeground = m_targetWindow == GetForegroundWindow();
    if (m_visible != isForeground && !m_alwaysOnTop)
    {
        m_visible = isForeground;
        changeFound = true;

        if (m_visible)
            setWndPosFlags |= SWP_SHOWWINDOW;
        else
            setWndPosFlags |= SWP_HIDEWINDOW;

        zOrderChange = HWND_TOP;
    } else {
        setWndPosFlags |= SWP_NOZORDER;
    }

    if (changeFound && m_hWnd)
        SetWindowPos(m_hWnd, zOrderChange, m_posX, m_posY, m_width, m_height, setWndPosFlags);

    // if size has changed, reset the overlay
    if (m_pDevice && !(setWndPosFlags & SWP_NOSIZE))
    {
        D3DPRESENT_PARAMETERS D3Dparams = getPresentParams();

        if (m_cbPreReset)
            m_cbPreReset();

        m_pDevice->Reset(&D3Dparams);

        if (m_cbPostReset)
            m_cbPostReset();

        resetRequiredSettings();
    }
}