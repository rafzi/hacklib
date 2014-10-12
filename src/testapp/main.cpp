#include <thread>
#include <chrono>

#include <Windows.h>
int main()
{
    LoadLibraryA("gw2hackd.dll");
    std::this_thread::sleep_for(std::chrono::hours(10));
}