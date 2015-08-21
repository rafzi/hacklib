#ifndef HACKLIB_GFXOVERLAY_H
#define HACKLIB_GFXOVERLAY_H

#include "hacklib/Handles.h"
#include <thread>
#include <future>


namespace hl {


class GfxOverlay
{
    friend class GfxOverlayImpl;

public:
    enum class Error
    {
        Okay,       // Success.
        Window,     // The window could not be created.
        Context,    // The graphics context could not be created.
        Other       // Other error.
    };

public:
    // Determines the current window manager mode.
    static bool IsCompositionEnabled();

public:
    GfxOverlay(ModuleHandle hModule = NULL);
    GfxOverlay(const GfxOverlay &) = delete;
    GfxOverlay &operator= (const GfxOverlay &) = delete;
    ~GfxOverlay();

    Error create(int posX, int posY, int width, int height);

    void close();
    bool isOpen() const;

    int getPosX() const;
    int getPosY() const;
    int getWidth() const;
    int getHeight() const;
    GraphicsContext getContext() const;

    void resetContext();
    // Helper for starting to draw. Also clears the backbuffer accordingly.
    void beginDraw();
    // Helper for finishing to draw by swapping backbuffer and frontbuffer.
    void swapBuffers();

private:
    void impl_construct();
    void impl_destruct();
    void impl_close();
    void impl_windowThread(std::promise<Error>& p);

protected:
    virtual void cbWindowLoop();

protected:
    class GfxOverlayImpl *m_impl = nullptr;

    std::thread m_thread;
    ModuleHandle m_hModule = NULL;
    WindowHandle m_hWnd = 0;
    bool m_isOpen = false;

    int m_posX = 0;
    int m_posY = 0;
    int m_width = 0;
    int m_height = 0;

};

}

#endif
