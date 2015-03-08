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

    // Create the overlay.
    // The overlay dissapears of the target window loses focus. Set
    // the parameter to true to prevent this.
    GfxOverlay::Error create(bool alwaysOnTop = false);

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
    bool m_alwaysOnTop = false;
    std::function <void()> m_cbPreReset;
    std::function <void()> m_cbPostReset;

};

}

#endif
