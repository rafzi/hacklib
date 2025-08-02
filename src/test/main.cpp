#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

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


int main(int argc, char* argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--child")
    {
        printf("Dummy executable running.\n");
        // Regularly return from syscalls to give ptrace a chance to wait.
        for (int i = 0; i < 10; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        printf("Dummy executable stopping.\n");
        return 0;
    }

#ifdef WIN32
    auto result = (void*)LoadLibrary("hl_test_hostd.dll");
#else
    auto result = dlopen("./libhl_test_hostd.so", RTLD_NOW | RTLD_LOCAL);
#endif

    if (result == NULL)
    {
        printf("Failed to load hl_test_host shared library\n");
        return 1;
    }

    std::cin.ignore();
}
