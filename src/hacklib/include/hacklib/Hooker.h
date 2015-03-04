#ifndef HACKLIB_HOOKER_H
#define HACKLIB_HOOKER_H

#include <cstdint>
#include <vector>
#include <memory>


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

    // Hook by replacing an object instances virtual table pointer.
    // This method can only target virtual functions. It should always
    // be preferred if possible as it is almost impossible to detect.
    // No real-only target memory is modified.
    const IHook *hookVT(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize = 1024);

    // Hook by patching the target location with a jump instruction.
    // Simple but has maximum flexibility. Your code has the responsibility
    // to resume execution somehow. A simple return will likely crash the target.
    const IHook *hookJMP(uintptr_t location, int nextInstructionOffset, void(*cbHook)());

    // Hook by patching the location with a jump like hookJMP, but jumps to
    // wrapper code that preserves registers, calls the given hook callback and
    // executes the overwritten instructions for maximum convenience.
    const IHook *hookDetour(uintptr_t location, int nextInstructionOffset, HookCallback_t cbHook);

    // Hook by using memory protection and a global exception handler.
    // This method is very slow.
    // No memory in the target is modified at all.
    const IHook *hookVEH(uintptr_t location, HookCallback_t cbHook);

    void unhook(const IHook *pHook);


private:
    std::vector<std::unique_ptr<IHook>> m_hooks;

};

}

#endif
