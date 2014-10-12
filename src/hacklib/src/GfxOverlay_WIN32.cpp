#include "hacklib/GfxOverlay.h"
#include <dwmapi.h>


static int g_wndClassRefCount = 0;

static const char WNDCLASS_OVERLAY[] = "GFXOVERLAY_1232";

static const int MSGTHREAD_SLEEP = 25;


bool GfxOverlay::IsCompositionEnabled()
{
    BOOL check;
    if (DwmIsCompositionEnabled(&check) != S_OK)
        return false;
    return check != FALSE;
}


GfxOverlay::GfxOverlay(HINSTANCE hModule)
{
    if (hModule)
        m_hModule = hModule;
    else
        m_hModule = GetModuleHandle(NULL);

    if (!g_wndClassRefCount++)
    {
        WNDCLASSEX wndClass = { };
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_HREDRAW|CS_VREDRAW;
        wndClass.lpfnWndProc = s_WndProc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = sizeof(this);
        wndClass.hInstance = m_hModule;
        wndClass.hIcon = NULL;                  // the icon will not be visible (WS_EX_NOACTIVATE, WS_POPUP)
        wndClass.hCursor = NULL;                // the cursor will not be visible (WS_EX_TRANSPARENT)
        wndClass.hbrBackground = NULL;          // tell windows that we draw the background
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = WNDCLASS_OVERLAY;
        wndClass.hIconSm = NULL;

        RegisterClassEx(&wndClass);
    }
}

GfxOverlay::~GfxOverlay()
{
    close();

    if (!--g_wndClassRefCount)
        UnregisterClass(WNDCLASS_OVERLAY, m_hModule);
}


GfxOverlay::Error GfxOverlay::create(int posX, int posY, int width, int height)
{
    // dont allow displaying twice
    close();

    m_posX = posX;
    m_posY = posY;
    m_width = width;
    m_height = height;

    // promise of return value of initialization
    std::promise<Error> p;
    auto ret = p.get_future();
    m_thread = std::thread(&GfxOverlay::threadProc, this, std::ref(p));

    // wait for initialization to be done and return false on error
    auto err = ret.get();
    if (err == Error::Success)
        m_isOpen = true;

    return err;
}


void GfxOverlay::close()
{
    if (m_isOpen)
        PostMessage(m_hWnd, WM_CLOSE, 0, 0);

    if (m_thread.joinable())
        m_thread.join();
}

bool GfxOverlay::isOpen() const
{
    return m_isOpen;
}


IDirect3DDevice9 *GfxOverlay::getDev() const
{
    return m_pDevice;
}

D3DPRESENT_PARAMETERS GfxOverlay::getPresentParams() const
{
    DWORD multiSampleQuality = 0;
    D3DFORMAT depthFormat = D3DFMT_D16;

    D3DDISPLAYMODE D3Ddm = { };
    if (m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &D3Ddm) == D3D_OK)
    {
        // use best multisampling available
        m_pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3Ddm.Format, TRUE, D3DMULTISAMPLE_NONMASKABLE, &multiSampleQuality);

        // use a better depth buffer if available and compatible with our backbuffer
        if (m_pD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3Ddm.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, D3DFMT_D24S8) == D3D_OK &&
            m_pD3D->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3Ddm.Format, D3DFMT_A8R8G8B8, D3DFMT_D24S8) == D3D_OK)
        {
            depthFormat = D3DFMT_D24S8;
        }
    }

    // the backbuffer will be created with the format D3DFMT_A8R8G8B8 so it has an alpha channel.
    // so when drawing something the alphachannel will not be calculated and combined to a opaque color.
    // instead the alpha value is written to the backbuffer and can be presented to the alphablendable frontbuffer.

    D3DPRESENT_PARAMETERS D3Dparams = { };
    D3Dparams.BackBufferWidth = 0;
    D3Dparams.BackBufferHeight = 0;
    D3Dparams.BackBufferFormat = D3DFMT_A8R8G8B8; // alpha is needed in the backbuffer
    D3Dparams.BackBufferCount = 0;
    D3Dparams.MultiSampleType = multiSampleQuality ? D3DMULTISAMPLE_NONMASKABLE : D3DMULTISAMPLE_NONE;
    D3Dparams.MultiSampleQuality = multiSampleQuality-1;
    D3Dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    D3Dparams.hDeviceWindow = m_hWnd;
    D3Dparams.Windowed = TRUE;
    D3Dparams.EnableAutoDepthStencil = TRUE;
    D3Dparams.AutoDepthStencilFormat = depthFormat;
    D3Dparams.Flags = 0;
    D3Dparams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    D3Dparams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    return D3Dparams;
}

int GfxOverlay::getPosX() const
{
    return m_posX;
}
int GfxOverlay::getPosY() const
{
    return m_posY;
}
int GfxOverlay::getWidth() const
{
    return m_width;
}
int GfxOverlay::getHeight() const
{
    return m_height;
}


void GfxOverlay::clearRenderTarget() const
{
    m_pDevice->Clear(0, 0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
}

void GfxOverlay::resetRequiredSettings() const
{
    // magic. combined with the stuff above, this makes the window background completly transparent.
    // when something is drawn to the window frontbuffer, it will be visibe tough and it can use transparency.

    MARGINS margs;
    margs.cxLeftWidth = m_posX;
    margs.cxRightWidth = m_width;
    margs.cyTopHeight = m_posY;
    margs.cyBottomHeight = m_height;
    DwmExtendFrameIntoClientArea(m_hWnd, &margs);

    // set correct values for alphablending

    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // turn lighting off by default

    m_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
}



LRESULT CALLBACK GfxOverlay::s_WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // if msg is WM_NCCREATE, associate instance ptr from createparameter to this window
    // note: WM_NCCREATE is not the first msg to be sent to a window
    if (msg == WM_NCCREATE) {
        LPCREATESTRUCT cs = reinterpret_cast<LPCREATESTRUCT>(lparam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    // get instance ptr associated with this window
    GfxOverlay *pInstance = reinterpret_cast<GfxOverlay*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    // call member function to handle messages indiviually for each instance
    // always check ptr because wndproc is called before association
    if (pInstance) {
        return pInstance->wndProc(hWnd, msg, wparam, lparam);
    }
    // else
    return DefWindowProc(hWnd, msg, wparam, lparam);
}


LRESULT GfxOverlay::wndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wparam, lparam);
}

void GfxOverlay::threadProc(std::promise<Error> &p)
{
    // create a layered window. a layered window has a frontbuffer that is alphablendable.

    m_hWnd = CreateWindowEx(
        WS_EX_LAYERED|          // window can be alphablended
        WS_EX_TRANSPARENT|      // window does not obscure any windows beneath eg clicks fall through
        WS_EX_TOPMOST|          // window placed and stays above all nontopmost windows
        WS_EX_NOACTIVATE,       // window does not appear in the taskbar
        WNDCLASS_OVERLAY, "Layer",
        WS_VISIBLE|             // window is initially visible
        WS_POPUP,               // window has no frame or border
        m_posX, m_posY, m_width, m_height, HWND_DESKTOP, NULL, m_hModule, this);

    if (!m_hWnd) {
        p.set_value(Error::Window);
        return;
    }

    // after the CreateWindowEx call, the layered window will not become visible until
    // the SetLayeredWindowAttributes or UpdateLayeredWindow function has been called for this window.

    // set the layered window to be fully opaque and not use a transparency key.
    // it still has a alphablendable frontbuffer though.

    if (!SetLayeredWindowAttributes(m_hWnd, 0, 255, LWA_ALPHA)) {
        p.set_value(Error::Other);
        return;
    }

    // create a usual d3d9 device. only special is the alphablendable backbuffer, see getPresentParams
    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D) {
        p.set_value(Error::Device);
        return;
    }

    D3DPRESENT_PARAMETERS D3Dparams = getPresentParams();

    if (m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_MULTITHREADED|D3DCREATE_HARDWARE_VERTEXPROCESSING, &D3Dparams, &m_pDevice) != D3D_OK &&
        m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_MULTITHREADED|D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3Dparams, &m_pDevice) != D3D_OK)
    {
        p.set_value(Error::Device);
        return;
    }

    resetRequiredSettings();

    p.set_value(Error::Success);

    MSG msg;
    do {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            cbWindowLoop();
            std::this_thread::sleep_for(std::chrono::milliseconds(MSGTHREAD_SLEEP));
        }
    } while (msg.message != WM_QUIT);

    if (m_pDevice)
        m_pDevice->Release();
    if (m_pD3D)
        m_pD3D->Release();

    m_isOpen = false;
}


void GfxOverlay::cbWindowLoop()
{

}