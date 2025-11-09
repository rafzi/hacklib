#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>
#include <filesystem>

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

    std::remove("hl_test_success");

    std::string error;
#ifdef WIN32
    auto result = (void*)LoadLibrary("hl_test_hostd.dll");
    if (!result)
    {
        DWORD code = GetLastError();
        LPVOID buf = NULL;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
        error = (char*)buf;
        LocalFree(buf);
    }
#else
    auto result = dlopen("./libhl_test_hostd.so", RTLD_NOW | RTLD_LOCAL);
    if (!result)
    {
        error = dlerror();
    }
#endif

    if (result == NULL)
    {
        printf("Failed to load hl_test_host shared library: %s\n", error.c_str());
        return 1;
    }

    int elapsedTime = 0;
    while (elapsedTime++ < 200)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (std::filesystem::exists("hl_test_success"))
        {
            std::remove("hl_test_success");
            return 0;
        }
    }

    printf("Timed out while waiting for tests to finish.\n");
    return 1;
}
