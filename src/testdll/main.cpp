#include "hacklib/Main.h"
#include "hacklib/D3DDeviceFetcher.h"
#include "hacklib/Hooker.h"
#include "hacklib/ConsoleEx.h"
#include <cstdio>


hl::StaticInit<class MyMain> g_main;


static void cbEndScene(hl::CpuContext *ctx);


void __declspec(naked) hkTest()
{
    for (;;);
}


const hl::IHook *vthk;
HRESULT __stdcall hkvtEndscene(LPDIRECT3DDEVICE9 pDevice)
{
    return ((HRESULT(__thiscall*)(LPDIRECT3DDEVICE9, LPDIRECT3DDEVICE9))(vthk->getLocation()))(pDevice, pDevice);
}


class MyMain : public hl::Main
{
public:
    bool init() override
    {
        auto p = hl::ConsoleEx::GetDefaultParameters();
        p.cellsYBuffer = 5000;
        m_con.create("TestDll", &p);

        auto dev = hl::D3DDeviceFetcher::GetD3D9Device(3000);

        m_con.printf("device: %08X\n", dev);

        MessageBoxA(0, 0, 0, 0);

        if (!dev)
            return false;

        vthk = m_hooker.hookVT((uintptr_t)dev, 42, (uintptr_t)hkvtEndscene);

        //uintptr_t endScene = ((uintptr_t**)dev)[0][42];
        //m_con.printf("endscene: %08X\n", endScene);

        //MessageBoxA(0, 0, 0, 0);
        //m_hooker.hookDetour(endScene, 7, cbEndScene);
        //m_hooker.hookJMP(endScene, 7, hkTest);

        return true;
    }

    bool step() override
    {
        if (GetAsyncKeyState(VK_HOME) < 0)
            return false;

        return true;
    }

    hl::ConsoleEx m_con;
    hl::Hooker m_hooker;

};



static void cbEndScene(hl::CpuContext *ctx)
{
    Sleep(100);
}