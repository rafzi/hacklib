#include "hacklib/D3DDeviceFetcher.h"
#include "hacklib/Hooker.h"
#include <mutex>
#include <condition_variable>
#include <Windows.h>


using namespace hl;


#ifdef ARCH_64BIT
#define REG_STACKPTR RSP
#else
#define REG_STACKPTR ESP
#endif


std::mutex g_mutex;
std::condition_variable g_condVar;
LPDIRECT3DDEVICE9 g_pDevice;


static void cbEndScene(hl::CpuContext *ctx)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_pDevice = *(LPDIRECT3DDEVICE9*)(ctx->REG_STACKPTR + sizeof(void*));
    g_condVar.notify_one();
}


IDirect3DDevice9 *D3DDeviceFetcher::GetD3D9Device(int timeout)
{
    // clean globals
    g_pDevice = NULL;

    // create a dummy d3d device
    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProc, 0, 0, GetModuleHandleA(NULL), 0, 0, 0, 0, "DXTMP", 0 };
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("DXTMP", 0, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), 0, wc.hInstance, 0);
    if (!hWnd) return NULL;
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return NULL;
    D3DPRESENT_PARAMETERS d3dPar = { 0 };
    d3dPar.Windowed = TRUE;
    d3dPar.SwapEffect = D3DSWAPEFFECT_DISCARD;
    LPDIRECT3DDEVICE9 pDev;
    pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPar, &pDev);
    if (!pDev) return NULL;

    // fetch the location of endscene in the d3d9 module via vtable of dummy device
    auto endScene = ((uintptr_t**)pDev)[0][42];

    // clean up dummy device
    pDev->Release();
    pD3D->Release();
    DestroyWindow(hWnd);
    UnregisterClassA("DXTMP", wc.hInstance);

    Hooker hooker;
    hooker.hookVEH(endScene, cbEndScene);

    std::unique_lock<std::mutex> lock(g_mutex);
    g_condVar.wait_for(lock, std::chrono::milliseconds(timeout), []{ return g_pDevice != NULL; });

    return g_pDevice;
}