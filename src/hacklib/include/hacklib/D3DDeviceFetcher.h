#ifndef D3DDEVICEFETCHER_H
#define D3DDEVICEFETCHER_H
// by Rafael S.

// requires dxsdk linked
#include "d3d9.h"

namespace D3DDeviceFetcher
{
    // when target does not call EndScene, the function returns null after timeout
    IDirect3DDevice9 *GetDeviceStealth(int timeout = 20000);
    
}

#endif