#include "hacklib/D3DDeviceFetcher.h"
#include <Windows.h>


long *g_pEndScene;
HANDLE g_hHookHit, g_hAbortHook;
LPDIRECT3DDEVICE9 g_pDevice;

LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exc);


IDirect3DDevice9 *D3DDeviceFetcher::GetDeviceStealth(int timeout)
{
    // clean globals
    g_pEndScene = NULL;
    g_hHookHit = NULL;
    g_hAbortHook = NULL;
    g_pDevice = NULL;

    // create a dummy d3d device
    WNDCLASSEXA wc = {sizeof(WNDCLASSEXA),CS_CLASSDC,DefWindowProc,0,0,GetModuleHandleA(NULL),0,0,0,0,"DXTMP",0};
    RegisterClassExA(&wc);
    HWND hWnd = CreateWindowA("DXTMP",0,WS_OVERLAPPEDWINDOW,100,100,300,300,GetDesktopWindow(),0,wc.hInstance,0);
    if (!hWnd) return NULL;
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return NULL;
    D3DPRESENT_PARAMETERS d3dPar = {0};
    d3dPar.Windowed = TRUE;
    d3dPar.SwapEffect = D3DSWAPEFFECT_DISCARD;
    LPDIRECT3DDEVICE9 pDev;
    pD3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,hWnd,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&d3dPar,&pDev);
    if (!pDev) return NULL;

    // fetch the location of endscene in the d3d9 module via vtable of dummy device
    g_pEndScene = reinterpret_cast<long***>(pDev)[0][42];

    // clean up dummy device
    pDev->Release();
    pD3D->Release();
    DestroyWindow(hWnd);
    UnregisterClassA("DXTMP",wc.hInstance);

    // hook endscene with veh
    PVOID pExHandler = AddVectoredExceptionHandler(1, VectoredHandler);
    if (!pExHandler) return NULL;
    g_hHookHit = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hAbortHook = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_hHookHit && g_hAbortHook)
    {
        DWORD orgProt = 0;
        if (!VirtualProtect(g_pEndScene, 1, PAGE_EXECUTE_READ|PAGE_GUARD, &orgProt)) return NULL;

        // wait for hook to get hit
        if (WaitForSingleObject(g_hHookHit, timeout) != WAIT_OBJECT_0) {
            // hook was not hit within <timeout> milliseconds
            SetEvent(g_hAbortHook);
            // wait one more second to be able to assume either the abort was registered
            // or hooked region does not get hit at all
            Sleep(1000);
        }

        // reset original memory protection in case handler was not called
        VirtualProtect(g_pEndScene, 1, orgProt, &orgProt);

        CloseHandle(g_hHookHit);
        CloseHandle(g_hAbortHook);
    }

    RemoveVectoredExceptionHandler(pExHandler);

    return g_pDevice;
}

LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exc)
{
    if (exc->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
    {
        // guard page exeption occured. guard page protection is gone now

        if (WaitForSingleObject(g_hAbortHook, 0) == WAIT_OBJECT_0) {
            // abort event was sent
            // dont do anything so guard page protection and single-step flag are gone
        } else if (g_pEndScene == reinterpret_cast<long*>(exc->ContextRecord->Eip)) {
            // hook was hit, catch device ptr from stack (first parameter of endscene function)
            g_pDevice = *reinterpret_cast<LPDIRECT3DDEVICE9*>(exc->ContextRecord->Esp + 0x4);
            SetEvent(g_hHookHit);
        } else {
            // hook was not hit and has to be refreshed. set single-step flag
            exc->ContextRecord->EFlags |= 0x100;
        }
        
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    if (exc->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
    {
        // single-step exeption occured. single-step flag is cleared now

        // set guard page protection
        DWORD oldProt;
        VirtualProtect(g_pEndScene, 1, PAGE_EXECUTE_READ|PAGE_GUARD, &oldProt);

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}