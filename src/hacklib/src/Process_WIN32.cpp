#include "hacklib/Process.h"
#include <Windows.h>
#include <stdexcept>


int hl::Process::join()
{
    if (!m_id)
    {
        throw std::runtime_error("Process is not joinable");
    }

    if (WaitForSingleObject((HANDLE)m_handle, INFINITE) != WAIT_OBJECT_0)
    {
        throw std::runtime_error("WaitForSingleObject failed");
    }

    DWORD exitCode;
    if (GetExitCodeProcess((HANDLE)m_handle, &exitCode) == 0)
    {
        throw std::runtime_error("GetExitCodeProcess failed");
    }

    m_id = 0;
    CloseHandle((HANDLE)m_handle);
    return exitCode;
}

hl::Process hl::LaunchProcess(const std::string& command, const std::vector<std::string>& args,
                              const std::string& initialDirectory)
{
    std::string cmdline = command;
    for (const auto& arg : args)
    {
        cmdline += " ";
        cmdline += arg;
    }

    const char* initialDirectoryCStr = initialDirectory.empty() ? NULL : initialDirectory.c_str();

    STARTUPINFOA startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    BOOL result = CreateProcessA(NULL, const_cast<char*>(cmdline.c_str()), NULL, NULL, false, 0, NULL,
                                 initialDirectoryCStr, &startupInfo, &processInfo);

    if (!result)
    {
        throw std::runtime_error("CreateProcess failed");
    }
    CloseHandle(processInfo.hThread);

    return Process(processInfo.dwProcessId, (uintptr_t)processInfo.hProcess);
}
