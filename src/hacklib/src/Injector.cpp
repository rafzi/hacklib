#include "hacklib/Injector.h"
#include <Windows.h>
#include <TlHelp32.h>


class Injection
{
public:
    Injection(std::string *error) : m_error(error)
    {
    }

    ~Injection()
    {
        if (m_remoteMem != NULL) {
            VirtualFreeEx(m_hProc, m_remoteMem, 0, MEM_RELEASE);
        }
        if (m_hRemoteThread != NULL) {
            CloseHandle(m_hRemoteThread);
        }
        if (m_hProc != NULL) {
            CloseHandle(m_hProc);
        }
    }

    void writeErr(const std::string& error)
    {
        if (m_error)
        {
            *m_error += error;
        }
    }

    bool findFile(const std::string& file)
    {
        LPCSTR path = file.data();

        m_fileName = new CHAR[MAX_PATH+1];
        DWORD written = GetFullPathName(path, MAX_PATH, m_fileName, NULL);
        if (written == 0) {
            writeErr("Fatal: Could not get the full library file name\n");
            return false;
        }

        m_fileNameSize = (written + 1) * sizeof(CHAR);

        WIN32_FIND_DATA info;
        HANDLE hFile = FindFirstFile(m_fileName, &info);
        if (hFile == INVALID_HANDLE_VALUE) {
            writeErr("Fatal: Could not find the library file\n");
            return false;
        }

        FindClose(hFile);

        return true;
    }
    bool findApi()
    {
        HMODULE hMod = GetModuleHandle("kernel32");
        if (hMod == NULL) {
            writeErr("Fatal: Could not get kernel32 module handle\n");
            return false;
        }
        m_apiLoadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryA");
        if (m_apiLoadLib == NULL) {
            writeErr("Fatal: Could not get LoadLibraryA api address\n");
            return false;
        }
        return true;
    }
    bool openProc(int pid)
    {
        m_hProc = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_VM_WRITE, FALSE, pid);
        if (m_hProc == NULL) {
            writeErr("Fatal: Could not open process\n");
            return false;
        }
        SYSTEM_INFO info;
        GetNativeSystemInfo(&info);
        if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            BOOL isWow64;
            if (!IsWow64Process(m_hProc, &isWow64)) {
                writeErr("Warning: Could not determine bitness of target process\n");
            } else {
#ifdef _WIN64
                if (isWow64) {
                    writeErr("Fatal: Can not inject into 32-bit process from 64-bit injector\n");
                    return false;
                }
#else
                if (!isWow64) {
                    writeErr("Fatal: Can not inject into 64-bit process from 32-bit injector\n");
                    return false;
                }
#endif
            }
        }
        return true;
    }
    bool remoteStoreFileName()
    {
        m_remoteMem = VirtualAllocEx(m_hProc, NULL, m_fileNameSize, MEM_COMMIT, PAGE_READWRITE);
        if (m_remoteMem == NULL) {
            writeErr("Fatal: Could not allocate remote memory\n");
            return false;
        }
        SIZE_T written;
        if (!WriteProcessMemory(m_hProc, m_remoteMem, m_fileName, m_fileNameSize, &written) || written != m_fileNameSize) {
            writeErr("Fatal: Could not write to remote memory\n");
            return false;
        }
        return true;
    }
    bool runRemoteThread()
    {
        m_hRemoteThread = CreateRemoteThread(m_hProc, NULL, 0, m_apiLoadLib, m_remoteMem, 0, NULL);
        if (m_hRemoteThread == NULL) {
            writeErr("Fatal: Could not create remote thread\n");
            return false;
        }
        if (!SetThreadPriority(m_hRemoteThread, THREAD_PRIORITY_TIME_CRITICAL)) {
            writeErr("Warning: Could not set remote thread priority");
        }
        if (WaitForSingleObject(m_hRemoteThread, 2000) != WAIT_OBJECT_0) {
            writeErr("Warning: The remote thread did not respond\n");
        } else {
            DWORD exitCode;
            if (!GetExitCodeThread(m_hRemoteThread, &exitCode)) {
                writeErr("Warning: Could not get remote thread exit code\n");
            } else {
                if (exitCode == 0) {
                    writeErr("Warning: Could not confirm that the library was loaded\n");
                }
            }
        }
        return true;
    }

private:
    LPSTR m_fileName = NULL;
    SIZE_T m_fileNameSize = 0;
    HANDLE m_hProc = NULL;
    HANDLE m_hRemoteThread = NULL;
    LPVOID m_remoteMem = NULL;
    LPTHREAD_START_ROUTINE m_apiLoadLib = NULL;

    std::string *m_error;

};


bool hl::Inject(int pid, const std::string& libFileName, std::string *error)
{
    Injection inj(error);

    return inj.findFile(libFileName) && inj.findApi() && inj.openProc(pid) && inj.remoteStoreFileName() && inj.runRemoteThread();
}

std::vector<int> hl::GetPIDsByProcName(std::string pname)
{
    std::vector<int> result;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return result;

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &entry))
    {
        do
        {
            if (pname == entry.szExeFile)
            {
                result.push_back(entry.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &entry));
    }

    return result;
}