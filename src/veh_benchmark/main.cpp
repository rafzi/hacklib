#include "hacklib/Timer.h"
#include <cstdint>
#include <vector>
#include <memory>
#include <map>
#include <thread>
#include <iostream>
#include <Windows.h>


#ifdef ARCH_64BIT
#define REG_INSTRUCTIONPTR Rip
#else
#define REG_INSTRUCTIONPTR Eip
#endif


namespace hl
{
    struct Hooker
    {
        typedef int* HookCallback_t;
    };
}


struct Page
{
    Page(uintptr_t begin, uintptr_t end) :
        m_begin(begin),
        m_end(end),
        m_refs(1)
    {
    }
    uintptr_t m_begin;
    uintptr_t m_end;
    int m_refs;
};

class VEHHookManager
{
public:
    VEHHookManager()
    {
        m_pages[0] = nullptr;
        if (!s_pageSize)
        {
            SYSTEM_INFO sys_info;
            GetSystemInfo(&sys_info);
            s_pageSize = sys_info.dwPageSize;
        }
    }
    hl::Hooker::HookCallback_t getHook(uintptr_t adr)
    {
        auto it = m_hooks.find(adr);
        if (it != m_hooks.end())
            return it->second;

        return nullptr;
    }
    Page *getPage(uintptr_t adr)
    {
        auto it = m_pages.upper_bound(adr);
        if (it != m_pages.end())
            return (--it)->second.get();

        return nullptr;
    }
    void addHook(uintptr_t adr, hl::Hooker::HookCallback_t cbHook)
    {
        m_hooks[adr] = cbHook;

        auto page = getPage(adr);
        if (page)
        {
            page->m_refs++;
        } else
        {
            MEMORY_BASIC_INFORMATION info;
            if (!VirtualQuery((LPVOID)adr, &info, sizeof(info)))
                throw std::runtime_error("VirtualQuery failed\n");

            uintptr_t lowerBound = (uintptr_t)info.BaseAddress;
            uintptr_t upperBound = lowerBound + s_pageSize;
            m_pages[lowerBound] = std::make_unique<Page>(lowerBound, upperBound);
            m_pages[upperBound] = nullptr;
        }
    }
    void removeHook(uintptr_t adr)
    {
        m_hooks.erase(adr);

        auto page = getPage(adr);
        if (page)
        {
            if (page->m_refs == 1)
            {
                m_pages.erase(page->m_end);
                m_pages.erase(page->m_begin);
            } else
            {
                page->m_refs--;
            }
        }
    }
private:
    static uintptr_t s_pageSize;
    std::map<uintptr_t, hl::Hooker::HookCallback_t> m_hooks;
    std::map<uintptr_t, std::unique_ptr<Page>> m_pages;
};


uintptr_t VEHHookManager::s_pageSize;
static VEHHookManager g_vehHookManager;


static void checkForHookAndCall(uintptr_t adr, CONTEXT *pCtx)
{
    // Check for hooks and call them.
    auto cbHook = g_vehHookManager.getHook(adr);
    if (cbHook)
    {
        // ~~~
    }
}


static LONG CALLBACK VectoredHandlerGuard(PEXCEPTION_POINTERS exc)
{
    static uintptr_t guardFaultAdr;

    CONTEXT *pCtx = exc->ContextRecord;

    if (exc->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
    {
        // Guard page exeption occured. Guard page protection is gone now.
        // Parameters: [0]: access type that caused the fault, [1]: address that was accessed

        // Save the address that was accessed to reprotect it at the single-step exception.
        guardFaultAdr = exc->ExceptionRecord->ExceptionInformation[1];

        // Set single-step flag to advance one instruction.
        pCtx->EFlags |= 0x100;

        // Only hook for execute access. Not if someone tries to read the hooked memory.
        if (exc->ExceptionRecord->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT)
        {
            checkForHookAndCall(guardFaultAdr, pCtx);
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    } else if (exc->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
    {
        // Single-step exeption occured. Single-step flag is cleared now.

        // Check if we are within a page that contains hooks.
        if (g_vehHookManager.getPage(pCtx->REG_INSTRUCTIONPTR))
        {
            // We are still on a hooked page. Continue single-stepping.
            pCtx->EFlags |= 0x100;

            checkForHookAndCall(pCtx->REG_INSTRUCTIONPTR, pCtx);
        } else
        {
            // We stepped out of a hooked page.
            // Check if the page that was last guarded still contains a hook.
            if (g_vehHookManager.getPage(guardFaultAdr))
            {
                // Restore guard page protection.
                DWORD oldProt;
                VirtualProtect((LPVOID)guardFaultAdr, 1, PAGE_EXECUTE_READ|PAGE_GUARD, &oldProt);
            }
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}


static LONG CALLBACK VectoredHandlerNoaccess(PEXCEPTION_POINTERS exc)
{
    static uintptr_t accessFaultAdr;

    DWORD oldProt;
    CONTEXT *pCtx = exc->ContextRecord;

    if (exc->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
    {
        // Access violation exception occured.
        // Parameters: [0]: access type that caused the fault, [1]: address that was accessed

        // Save the address that was accessed to reprotect it at the single-step exception.
        accessFaultAdr = exc->ExceptionRecord->ExceptionInformation[1];

        // Set single-step flag to advance one instruction.
        pCtx->EFlags |= 0x100;

        // Only hook for execute access. Not if someone tries to read the hooked memory.
        if (exc->ExceptionRecord->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT)
        {
            checkForHookAndCall(accessFaultAdr, pCtx);
        }

        // Make the page accessible again so we can single-step.
        VirtualProtect((LPVOID)accessFaultAdr, 1, PAGE_EXECUTE_READ, &oldProt);

        return EXCEPTION_CONTINUE_EXECUTION;
    } else if (exc->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
    {
        // Single-step exeption occured. Single-step flag is cleared now.

        // We stepped out of a hooked page.
        // Check if the page that was last protected still contains a hook.
        if (g_vehHookManager.getPage(accessFaultAdr))
        {
            // Restore page protection.
            VirtualProtect((LPVOID)accessFaultAdr, 1, PAGE_NOACCESS, &oldProt);
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}


typedef void(*func_t)();


#pragma comment(linker, "/INCREMENTAL:NO")
void assembleMe()
{
    volatile int counter = 10000;
    while (--counter);
}


int main()
{
    DWORD dwOldProt;

    // Setup dummy codepage that loops on incrementing the counter.
    uint8_t *mem1 = (uint8_t*)VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    uintptr_t hookPage = (uintptr_t)mem1;
    func_t f = (func_t)mem1;

    uintptr_t code = (uintptr_t)assembleMe;
    memcpy(mem1, (void*)code, 0x100);

    // Test 1: Guard page protection + single-step optimization.

    g_vehHookManager.addHook(hookPage, nullptr);
    VirtualProtect((LPVOID)hookPage, 1, PAGE_EXECUTE_READ|PAGE_GUARD, &dwOldProt);
    auto handler = AddVectoredExceptionHandler(1, VectoredHandlerGuard);

    hl::Timer tGuard;
    f();
    printf("time elapsed for test 1: %f\n", tGuard.diff());


    // Reset.
    g_vehHookManager.removeHook(hookPage);
    Sleep(500);
    RemoveVectoredExceptionHandler(handler);


    // Test 2: No access protection.

    g_vehHookManager.addHook(hookPage, nullptr);
    VirtualProtect((LPVOID)hookPage, 1, PAGE_NOACCESS, &dwOldProt);
    handler = AddVectoredExceptionHandler(1, VectoredHandlerNoaccess);

    hl::Timer tNoacc;
    f();
    printf("time elapsed for test 2: %f\n", tNoacc.diff());


    std::cin.ignore();
}