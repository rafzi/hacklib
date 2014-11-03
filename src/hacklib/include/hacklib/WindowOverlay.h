#ifndef HACKLIB_WINDOWOVERLAY_H
#define HACKLIB_WINDOWOVERLAY_H

#include "GfxOverlay.h"


namespace hl {


/*
 * Wrapper for GfxOverlay that follows the main window of the current process.
 */
class WindowOverlay : public GfxOverlay
{
public:
    WindowOverlay(HINSTANCE hModule = NULL);

    GfxOverlay::Error create();

    // When the host window is resized, the device must be reset.
    // This function registers hooks before and after the Reset operation.
    // For example see hl::Drawer::OnDeviceLost and OnDeviceReset.
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

}

#endif
