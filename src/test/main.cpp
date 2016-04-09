#include <thread>
#include <chrono>
#include <iostream>
#include <cstdio>

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif


/*
This program exists because of the runtime linking behaviour of linux.
If hl_test_host would be an executable, it would share the hacklib code and
data with hl_test_lib. This would cause hacklib to misbehave.
*/


int main(int argc, char *argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--child")
    {
        printf("Dummy executable running.\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        printf("Dummy executable stopping.\n");
        return 0;
    }

#ifdef WIN32
    LoadLibrary("hl_test_hostd.dll");
#else
    dlopen("./libhl_test_hostd.so", RTLD_NOW | RTLD_LOCAL);
#endif

    std::cin.ignore();
}
