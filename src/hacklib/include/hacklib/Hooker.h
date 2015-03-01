#ifndef HACKLIB_HOOKER_H
#define HACKLIB_HOOKER_H

#include <cstdint>
#include <vector>
#include <memory>
#include <map>


namespace hl {


class Hooker;


class IHook
{
public:
    virtual ~IHook() {}
    virtual uintptr_t getLocation() const = 0;
};


struct CpuContext_x86
{
    uintptr_t EIP;
    uintptr_t EFLAGS;
    uintptr_t EDI;
    uintptr_t ESI;
    uintptr_t EBP;
    uintptr_t ESP;
    uintptr_t EBX;
    uintptr_t EDX;
    uintptr_t ECX;
    uintptr_t EAX;
};

struct CpuContext_x86_64
{
    uintptr_t RIP;
    uint32_t EFLAGS;
    uintptr_t R15;
    uintptr_t R14;
    uintptr_t R13;
    uintptr_t R12;
    uintptr_t R11;
    uintptr_t R10;
    uintptr_t R9;
    uintptr_t R8;
    uintptr_t RDI;
    uintptr_t RSI;
    uintptr_t RBP;
    uintptr_t RSP;
    uintptr_t RBX;
    uintptr_t RDX;
    uintptr_t RCX;
    uintptr_t RAX;
};

#ifdef ARCH_64BIT
typedef CpuContext_x86_64 CpuContext;
#else
typedef CpuContext_x86 CpuContext;
#endif


class Hooker
{
public:
    typedef void(__cdecl* HookCallback_t)(CpuContext *);

    const IHook *hookVT(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize = 1024);

    const IHook *hookJMP(uintptr_t location, int nextInstructionOffset, void(*cbHook)());

    const IHook *hookDetour(uintptr_t location, int nextInstructionOffset, HookCallback_t cbHook);

    const IHook *hookVEH(uintptr_t location, HookCallback_t cbHook);

    void unhook(const IHook *pHook);


    std::map<uintptr_t, HookCallback_t> m_vehCallbacks;

private:
    std::map<uintptr_t, std::vector<uintptr_t>> m_fakeVTs;

    std::vector<std::unique_ptr<IHook>> m_hooks;

};

}

#endif
