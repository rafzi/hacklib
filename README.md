# Hacklib #

Hacklib is a C++ library for building applications that run as a shared library in another application. It provides general purpose functionality like pattern scanning, hooking and laying out foreign classes. Additionally it contains some D3D and OpenGL drawing facilities and a cross-platform, high-performance, 3D-capable, transparent overlay.

Every component in this project can target 32-bit x86 Windows and most of it targets 64-bit x86_64 Windows as well. Some stuff should work on every platform that has a modern C++ compiler.

## Example projects ##

This repository contains a couple simple examples already:

* `injector`: A command line application to inject shared libraries into processes. This is the tool to get your projects into target applications.
* `disableGfx`: A simple project that may be able to double your FPS in D3D9 games. But at what cost?
* `veh_benchmark`: Comparison of VEH hooking implementations.

Bigger examples are located in separate repositories:

* [hacklib_csgo](https://bitbucket.org/rafzi/hacklib_csgo): A minimal example for a real-world target Counter-Strike: Global Offensive. (Cross-platform)
* [hacklib_gw2](https://bitbucket.org/rafzi/hacklib_gw2): Graphical information gathering tool for Guild Wars 2.
* [hacklib_bf](https://bitbucket.org/rafzi/hacklib_bf): A game hack for the Battlefield series.

## Features ##

### Main.h ###

A helper / framework for implementing a shared library that runs by itself after it is loaded into the target process.


```
#!c++

class MyMain : public hl::Main
{
public:
    bool init() override
    {
        // Your init code here. Return true to loop on step member function afterwards.
        return false;
    }
};

hl::StaticInit<MyMain> g_main;
```


### WindowOverlay.h ###

A cross-platform high performance 3D-capable transparent overlay. Will follow a target window and always draw on top of it with the ability have transparency on the overlay. Can be rendered to with D3D on Windows and OpenGL on Linux. Requires the compositing window manager introduced with Windows Vista (always active since Windows 8). The X Window System is required on Linux.

For an example see Drawer.h section below.

### IDrawer.h / DrawerD3D.h / DrawerOpenGL.h ###

Wrapper for drawing with D3D or OpenGL in a resource-safe C++ way.


```
#!c++

hl::DrawerD3D drawer;
hl::WindowOverlay overlay;

if (overlay.create() != hl::WindowOverlay::Error::Okay)
    return false;

overlay.registerResetHandlers([&]{ drawer.onLostDevice(); }, [&]{ drawer.onResetDevice(); });
drawer.setContext(overlay.getContext());

while (true)
{
    overlay.beginDraw();
    drawer.drawCircle(100, 100, 30, hl::Color(150, 255, 100, 50));
    overlay.swapBuffers();
}
```


### Hooker.h ###

Implements various hooking methods like simple JMP redirection, convenient JMP detours, virtual table hooks and vectored exception handler hooking.

The implemented VEH hooking mechanism is about 3x faster than the conventional `PAGE_NOACCESS` implementation. See the project `veh_benchmark` for a comparison.

### PatternScanner.h ###

Provides pattern scanning techniques like masked search strings or search by referenced strings in the code of the target process.


```
#!c++

uintptr_t MapIdSig = hl::FindPattern("00 ?? 08 00 89 0d");

hl::PatternScanner scanner;
auto results = scanner.find({
    "ViewAdvanceDevice",
    "ViewAdvanceAgentSelect",
    "ViewAdvanceAgentView"
});

void *pAgentSelectionCtx = *(void**)(hl::FollowRelativeAddress(results[1] + 0xa) + 0x1);
```


### Logging.h ###

Convenient printf-like logging macros. Source information with file name, line number and function name are included in debug builds.

```
#!c++

// The default configuration also logs to a file next to the library.
hl::LogConfig logCfg;
logCfg.logFunc = [](const std::string& msg){ std::cout << msg << std::flush; };
hl::ConfigLog(logCfg);

HL_LOG_DBG("This is only shown in debug builds. printf formatting: %i\n", 3);
HL_LOG_ERR("This is always shown\n");
HL_LOG_RAW("This will log just the given input without source information or time\n");
```


### Memory.h ###

Various utilities for memory allocation, protection and mappings.


### Patch.h ###

Object wrapper around a simple code patch. Takes care of memory protection and restores everything on destruction.

```
#!c++

hl::Patch p1, p2;
p1.apply(0x00111111, (uint8_t)0xeb);
p2.apply(0x00222222, "\x90\x90\x90", 3);
```


### Injector.h ###

A way to forcibly get your shared library into the target process. For a tool building on this functionality see the `ldr` project.


### ConsoleEx.h ###

A high performance Windows console that accepts input and output simultaneously.


### D3DDeviceFetcher.h ###

Finds the D3D device used for rendering by the host application in a generic way.


```
#!c++

auto pDev = hl::D3DDeviceFetcher::GetD3D9Device();
```


### ForeignClass.h ###

Helper class for accessing a foreign class dynamically at runtime, including doing virtual member function calls.


```
#!c++

void *ptr = 0x12345678;
hl::ForeignClass obj = ptr;
int result = obj.call<int>(0x18, "Hello World", 3.14f, 42);
double value = obj.get<double>(0x6c);
obj.set(0x70, value+7);
```


### ImplementMember.h ###

Macros for declaring a foreign class statically in a type-safe, const-safe and simply convenient manner.


```
#!c++

class CPlayer
{
    // Declare virtual table function by offset
    IMPLVTFUNC(double, someFunc, 0x1c, int, id, double, speed);
    // Declare virtual table function by ordinal
    IMPLVTFUNC_OR(void, boop, 3);
    // Declare member variable by offset
    IMPLMEMBER(int, Id, 0x10);
    // Declare member variable by relative offset
    IMPLMEMBER_REL(D3DXVECTOR3, Pos, 0x8, Id);
    IMPLMEMBER_REL(float, Hp, 0, Pos);
};
```


### CrashHandler.h ###

A wrapper that catches system exceptions like memory access faults. The handlers should never be called in working programs, because C++ destructors are not called when a fault occurs.

```
#!c++

hl::CrashHandler([]{
    int crash = *(volatile int*)nullptr;
}, [](uint32_t code){
    printf("Crash prevented. code: %08X\n", code);
});
```


### ExeFile.h ###

An abstraction for PE or ELF executable images.


### Utility ###

These are not really related to the topic of this library, but might often be used in a program built from this library.

Rng.h:

```
#!c++

hl::Rng rng;

while (true)
{
    // From 0 to 100
    int a = rng.nextInt(0, 100);
    // From 7.1 to 10.3
    auto b = rng.nextReal(7.1, 10.3);
}
```

Timer.h:

```
#!c++

hl::Timer t;
// Some computation.
std::cout << t.diff() << std::endl;
t.reset();
// Another computation.
std::cout << t.diff() << std::endl;
```

Input.h:

```
#!c++

hl::Input input;

while (true)
{
    input.update();
    if (input.isDown(VK_SHIFT))
    {
        if (input.wentDown('F'))
        {
            // Shift was held and F changed state from not pressed to pressed.
        }
    }
}
```


## Dependencies ##

Hacklib is written in modern C++ and requires a recent compiler like Visual Studio 2015, GCC 4.8 or Clang 3.3 with C++14 support enabled. Use the ``vs2013`` tag for an outdated version that works for Visual Studio 2013.

The project is using CMake and version 2.8.11.2 or newer is required to build.

Graphics related components require the DirectX SDK June 2010 on Windows or X11, OpenGL libraries on Linux. The essential headers and libraries of the DirectX SDK are included in this repository. Required Linux packages would for example be on Debian/Ubuntu: `sudo apt-get install libx11-dev mesa-common-dev libglu1-mesa-dev libxrender-dev libxfixes-dev libglew-dev`.

## How to build ##

* Run CMake
* Build the project that was generated by CMake
* For your own project folders, it is most convenient to put them into the hacklib/src folder. Then provide a CMakeLists file in this folder. For example: hacklib/src/YourProject/CMakeLists.txt. You need to re-run CMake manually after adding your project folder, it will not detect the new folder itself. The basic CMakeLists file is the following, add source files as neccessary to the ADD_LIBRARY command:


```
#!cmake

PROJECT(YourProject)

ADD_LIBRARY(${PROJECT_NAME} SHARED main.cpp)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} hacklib)
```


## Contribute ##

Please use the issue tracker and submit pull requests to contribute.

## License ##

Free to use for any purpose. Don't claim you wrote the code or modified versions. If you release code or binaries give credit and link to this repository.

If you are making money with this code or have some spare money, please give me some!

[Paypal Donate Link](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=EW66GFAYT9K9C)