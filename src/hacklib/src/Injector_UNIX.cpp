#include "hacklib/Injector.h"
#include "hacklib/ExeFile.h"
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <algorithm>


#ifdef ARCH_64BIT
#define USER_REG_IP(u) u.regs.rip
#define USER_REG_SP(u) u.regs.rsp
#define USER_REG_AX(u) u.regs.rax
#else
#define USER_REG_IP(u) u.regs.eip
#define USER_REG_SP(u) u.regs.esp
#define USER_REG_AX(u) u.regs.eax
#endif


struct MemoryRegion
{
    uintptr_t base;
    size_t size;
    bool readable;
    bool writable;
    bool executable;
    std::string mappedFile;
};

static std::vector<MemoryRegion> GetProcMap(int pid)
{
    std::vector<MemoryRegion> result;

    char fileName[32];
    sprintf(fileName, "/proc/%d/maps", pid);

    std::ifstream file(fileName);

    unsigned long long start, end;
    char flags[32];
    unsigned long long file_offset;
    int dev_major, dev_minor;
    unsigned long long inode;
    char path[512];

    std::string line;
    while (std::getline(file, line))
    {
        path[0] = '\0';
        sscanf(line.c_str(), "%Lx-%Lx %31s %Lx %x:%x %Lu %511s", &start, &end, flags, &file_offset, &dev_major, &dev_minor, &inode, path);

        result.resize(result.size() + 1);
        auto region = result.rbegin();
        region->base = (uintptr_t)start;
        region->size = (size_t)(end - start);
        region->readable = flags[0] == 'r';
        region->writable = flags[1] == 'w';
        region->executable = flags[2] == 'x';
        region->mappedFile = path;
    }

    return result;
}


class Injection
{
public:
    Injection(std::string *error) : m_error(error)
    {
    }
    ~Injection()
    {
        if (m_pid)
        {
            if (m_remoteLibName)
            {
                // Free the remote memory for the library name.
                call(m_free, m_remoteLibName);
            }

            if (m_restoreBackup)
            {
                // Restore registers.
                memcpy(&m_regs, &m_regsBackup, sizeof(m_regs));
                setRegs();

                // Resume normal execution.
                resume();
            }

            ptrace(PTRACE_DETACH, m_pid, NULL, NULL);
        }
    }

    void writeErr(const std::string& error)
    {
        if (m_error)
        {
            *m_error += error;
        }
    }

    bool findFile(const std::string& fileName)
    {
        if (!realpath(fileName.c_str(), m_fileName))
        {
            writeErr("Fatal: Could not get the full library name\n");
            return false;
        }

        if (access(m_fileName, F_OK) == -1)
        {
            writeErr("Fatal: Could not find the library file\n");
            return false;
        }

        return true;
    }

    bool openProcess(int pid)
    {
        if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
        {
            writeErr("Fatal: Could not attach with ptrace\n");
            return false;
        }

        m_pid = pid;

        if (!wait())
            return false;
        if (!getRegs())
            return false;

        // Backup registers for restoring afterwards.
        memcpy(&m_regsBackup, &m_regs, sizeof(m_regs));

        // Don't touch 128 byte System V ABI redzone. Account for arguments that will be written after the SP.
        USER_REG_SP(m_regs) -= 256;

        m_restoreBackup = true;

        return true;
    }

    bool findLibs()
    {
        auto procMap = GetProcMap(m_pid);

        auto findLib = [&](const std::string& libName){
            return std::find_if(procMap.begin(), procMap.end(), [&](const MemoryRegion& r){
                auto pos = r.mappedFile.find(libName);
                if (pos != std::string::npos)
                {
                    pos += libName.size();
                    return r.mappedFile[pos] == '.' || r.mappedFile[pos] == '-';
                }
                return false;
            });
        };

        auto libc = findLib("libc");
        auto libdl = findLib("libdl");
        if (libc == procMap.end() || libdl == procMap.end())
        {
            writeErr("Fatal: Did not find mapping for libc and libdl libraries\n");
            return false;
        }

        m_libc = *libc;
        m_libdl = *libdl;

        return true;
    }

    bool findApis()
    {
        hl::ExeFile libc;
        libc.loadFromFile(m_libc.mappedFile);
        m_malloc = libc.getExport("malloc");
        m_free = libc.getExport("free");

        if (!m_malloc || !m_free)
        {
            writeErr("Fatal: Did not find exports for malloc, free\n");
            return false;
        }
        m_malloc += m_libc.base;
        m_free += m_libc.base;

        hl::ExeFile libdl;
        libdl.loadFromFile(m_libdl.mappedFile);
        m_dlopen = libdl.getExport("dlopen");
        m_dlclose = libdl.getExport("dlclose");

        if (!m_dlopen || !m_dlclose)
        {
            writeErr("Fatal: Did not find exports for dlopen, dlclose\n");
            return false;
        }
        m_dlopen += m_libdl.base;
        m_dlclose += m_libdl.base;

        return true;
    }

    bool remoteLoadLib()
    {
        size_t libNameLen = strlen(m_fileName) + 1;
        size_t paddedLibNameLen = ((libNameLen - 1) & ~(sizeof(uintptr_t) - 1)) + sizeof(uintptr_t);

        // Resolve weird state after syscall.
        if (!call(0, 0, 0))
            return false;

        // Allocate memory in the target process.
        if (!call(m_malloc, paddedLibNameLen))
            return false;

        m_remoteLibName = USER_REG_AX(m_regs);
        if (!m_remoteLibName)
        {
            writeErr("Fatal: malloc failed in the remote process\n");
            return false;
        }

        // Copy the library name to the remote process. Assume that PATH_MAX is padded to pointer size boundary.
        for (uintptr_t i = 0; i < paddedLibNameLen; i += sizeof(uintptr_t))
        {
            uintptr_t value = *(uintptr_t*)&m_fileName[i];
            if (ptrace(PTRACE_POKEDATA, m_pid, m_remoteLibName + i, (void*)value) < 0)
            {
                writeErr("Fatal: ptrace POKEDATA failed\n");
                return false;
            }
        }

        // Load the library from the target process.
        if (!call(m_dlopen, m_remoteLibName, RTLD_NOW | RTLD_GLOBAL))
            return false;

        // Check whether dlopen succeeded.
        if (!USER_REG_AX(m_regs))
        {
            writeErr("Fatal: Remote process could not load library\n");
            return false;
        }

        return true;
    }

private:
    bool getRegs()
    {
        if (ptrace(PTRACE_GETREGS, m_pid, NULL, &m_regs) < 0)
        {
            writeErr("Fatal: ptrace GETREGS failed\n");
            return false;
        }

        return true;
    }
    bool setRegs()
    {
        if (ptrace(PTRACE_SETREGS, m_pid, NULL, &m_regs) < 0)
        {
            writeErr("Fatal: ptrace SETREGS failed\n");
            return false;
        }

        return true;
    }
    bool call(uintptr_t function, uintptr_t arg1, uintptr_t arg2 = 0)
    {
        // The stack must be aligned on 16 byte boundary when a CALL is done.
        // Since no CALL is executed and a CALL pushes the return value, increment the SP.
        USER_REG_SP(m_regs) &= ~(uintptr_t)0xf;
        USER_REG_SP(m_regs) += sizeof(uintptr_t);

        // Write zero to top of stack, so that a return from a called function will trigger SIGSEGV.
        if (ptrace(PTRACE_POKEDATA, m_pid, USER_REG_SP(m_regs), (void*)0) < 0)
        {
            writeErr("Fatal: ptrace POKEDATA failed\n");
            return false;
        }

        USER_REG_IP(m_regs) = function;
#ifdef ARCH_64BIT
        m_regs.regs.rdi = arg1;
        m_regs.regs.rsi = arg2;
#else
        if (ptrace(PTRACE_POKEDATA, m_pid, USER_REG_SP(m_regs) + sizeof(uintptr_t), (void*)arg1) < 0)
        {
            writeErr("Fatal: ptrace POKEDATA failed\n");
            return false;
        }
        if (ptrace(PTRACE_POKEDATA, m_pid, USER_REG_SP(m_regs) + 2*sizeof(uintptr_t), (void*)arg2) < 0)
        {
            writeErr("Fatal: ptrace POKEDATA failed\n");
            return false;
        }
#endif

        if (!setRegs())
            return false;
        if (!resume())
            return false;
        if (!wait())
            return false;
        if (!getRegs())
            return false;

        return true;
    }
    bool singlestep()
    {
        if (ptrace(PTRACE_SINGLESTEP, m_pid, NULL, NULL) < 0)
        {
            writeErr("Fatal: ptrace SINGLESTEP failed\n");
            return false;
        }

        return true;
    }
    bool resume()
    {
        if (ptrace(PTRACE_CONT, m_pid, NULL, NULL) < 0)
        {
            writeErr("Fatal: ptrace continue failed\n");
            return false;
        }

        return true;
    }
    bool wait()
    {
        int status = 0;
        if (waitpid(m_pid, &status, 0) < 0)
        {
            writeErr("Fatal: waitpid failed\n");
            return false;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            writeErr("Fatal: Process is gone\n");
            return false;
        }

        return true;
    }

private:
    int m_pid = 0;
    char m_fileName[PATH_MAX];
    MemoryRegion m_libc, m_libdl;
    uintptr_t m_malloc, m_free, m_dlopen, m_dlclose;
    struct user m_regs, m_regsBackup;
    bool m_restoreBackup = false;
    uintptr_t m_remoteLibName = 0;

    std::string *m_error;

};


bool hl::Inject(int pid, const std::string& libFileName, std::string *error)
{
    Injection inj(error);

    return inj.findFile(libFileName) && inj.openProcess(pid) && inj.findLibs() && inj.findApis() && inj.remoteLoadLib();
}

std::vector<int> hl::GetPIDsByProcName(const std::string& pname)
{
    std::vector<int> result;
    return result;
}
