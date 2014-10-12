#ifndef WINDOWOVERLAY_H
#define WINDOWOVERLAY_H

#include "GfxOverlay.h"


class WindowOverlay : public GfxOverlay
{
public:
    WindowOverlay(HINSTANCE hModule = NULL);

    GfxOverlay::Error create();

    template <class F1, class F2>
    void registerResetHandlers(F1 cbPreReset, F2 cbPostReset)
    {
        m_cbPreReset = cbPreReset;
        m_cbPostReset = cbPostReset;
    }

private:
    virtual void cbWindowLoop();

private:
    HWND m_targetWindow = NULL;
    bool m_visible = false;
    std::function <void()> m_cbPreReset;
    std::function <void()> m_cbPostReset;

};

#endif
