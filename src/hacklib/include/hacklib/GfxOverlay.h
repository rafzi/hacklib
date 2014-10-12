#ifndef GFXOVERLAY_H
#define GFXOVERLAY_H

// link with: d3d9.lib dwmapi.lib
#include <Windows.h>
#include "d3d9.h"
#include <thread>
#include <future>


// the overlay requires the desktop window manager introduced in windows 6.0 (vista) to be in compositing mode
// otherwise the overlay will not be transparent, but still "works"
// in windows 6.0 (vista) and 6.1 (win7) the DWM can be in compositing (new) or stacking (old) mode
// in window 6.2 (win8) the DWM can only be in compositing mode
// overlay has to be created newly if composition mode changes


class GfxOverlay
{
    GfxOverlay(const GfxOverlay &) = delete;
    GfxOverlay &operator= (const GfxOverlay &) = delete;

public:
    enum class Error
    {
        Success,    // success
        Window,     // the window could not be created
        Device,     // the device could not be created
        Other       // other error
    };

public:
    static bool IsCompositionEnabled();

public:
    GfxOverlay(HINSTANCE hModule = NULL);
    ~GfxOverlay();

    Error create(int posX, int posY, int width, int height);

    void close();
    bool isOpen() const;

    IDirect3DDevice9 *getDev() const;
    D3DPRESENT_PARAMETERS getPresentParams() const;
    int getPosX() const;
    int getPosY() const;
    int getWidth() const;
    int getHeight() const;

    //void handleWindowMessages() const;
    void clearRenderTarget() const;
    void resetRequiredSettings() const;

private:
    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:
    // member functions to process window and thread callbacks
    LRESULT wndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void threadProc(std::promise<Error> &p);

protected:
    virtual void cbWindowLoop();

protected:
    std::thread m_thread;
    HINSTANCE m_hModule = NULL;
    HWND m_hWnd = NULL;
    IDirect3D9 *m_pD3D = nullptr;
    IDirect3DDevice9 *m_pDevice = nullptr;
    bool m_isOpen = false;

    int m_posX = 0;
    int m_posY = 0;
    int m_width = 0;
    int m_height = 0;

};

#endif
