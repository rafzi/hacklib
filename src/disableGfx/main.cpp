#include "hacklib/Main.h"
#include "hacklib/ConsoleEx.h"
#include "hacklib/D3DDeviceFetcher.h"
#include "hacklib/Hooker.h"
#include <cstdio>


hl::StaticInit<class MyMain> g_main;
const hl::IHook* g_indexedHook;


long __stdcall cbDrawIndexed(uintptr_t pInst, long a1, long a2, long a3, long a4, long a5, long a6)
{
    if (rand() % 2)
    {
        // Call the original function.
        auto orgFunc = g_indexedHook->getLocation();
        return ((long(__thiscall*)(uintptr_t, uintptr_t, long, long, long, long, long, long))orgFunc)(
            pInst, pInst, a1, a2, a3, a4, a5, a6);
    }
    else
    {
        // Don't do anything. Return D3D_OK to signal success.
        return 0;
    }
}

long __stdcall cbDrawIndexed11(uintptr_t pInst, long a1, long a2, long a3)
{
    return 0;
}
long __stdcall cbDraw11(uintptr_t pInst, long a1, long a2)
{
    return 0;
}


class MyMain : public hl::Main
{
public:
    bool init() override
    {
        m_con.create("disabler");

        auto coms = hl::D3DDeviceFetcher::GetD3D11Device(3000);

        m_hooker.hookVT((uintptr_t)coms.pDeviceContext, 13, (uintptr_t)cbDraw11);

        m_con.printf("sc: %p\ndev: %p\ndc: %p\n", coms.pSwapChain, coms.pDevice, coms.pDeviceContext);

        return true;

        auto dev = hl::D3DDeviceFetcher::GetD3D9Device(3000);

        // Hook IDirect3DDevice9::DrawIndexedPrimitive. This is usually the most used draw API.
        g_indexedHook = m_hooker.hookVT((uintptr_t)dev, 82, (uintptr_t)cbDrawIndexed);

        return true;
    }

    bool step() override
    {
        if (GetAsyncKeyState(VK_HOME) < 0)
            return false;

        return true;
    }

    hl::Hooker m_hooker;
    hl::ConsoleEx m_con;
};