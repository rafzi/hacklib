#ifndef HACKLIB_HANDLES_H
#define HACKLIB_HANDLES_H


namespace hl {


#ifdef _WIN32

#include <Windows.h>
#include <d3d9.h>
typedef HINSTANCE ModuleHandle;
typedef HWND WindowHandle;
typedef IDirect3DDevice9* GraphicsContext;

#else

typedef void* ModuleHandle;
// Must be compatible with X11 Window type.
typedef long unsigned int WindowHandle;
typedef void* GraphicsContext;

#endif

}

#endif
