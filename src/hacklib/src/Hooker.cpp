#include "hacklib/Hooker.h"
#include "hacklib/ExecutableAllocator.h"
#include <Windows.h>
#include <algorithm>
#include <mutex>


using namespace hl;


class VTHook : public IHook
{
public:
    VTHook(uintptr_t classInstance, int functionIndex) :
        pVT((uintptr_t**)classInstance),
        pOrgVT(*pVT),
        functionIndex(functionIndex)
    {
    }
    ~VTHook() override
    {
        *pVT = pOrgVT;
    }

    uintptr_t getLocation() const override
    {
        return pOrgVT[functionIndex];
    }

    uintptr_t **pVT;
    uintptr_t *pOrgVT;
    int functionIndex;
};

class JMPHook : public IHook
{
public:
    JMPHook(uintptr_t location, int offset) :
        location(location),
        offset(offset),
        backupCode(offset, 0xcc)
    {
    }
    ~JMPHook() override
    {
        DWORD dwOldProt;
        if (VirtualProtect((LPVOID)location, offset, PAGE_EXECUTE_READWRITE, &dwOldProt))
        {
            memcpy((void*)location, backupCode.data(), offset);
            VirtualProtect((LPVOID)location, offset, dwOldProt, &dwOldProt);
        }
    }

    uintptr_t getLocation() const override
    {
        return location;
    }

    uintptr_t location;
    int offset;
    std::vector<unsigned char> backupCode;
};

class DetourHook : public IHook
{
public:
    DetourHook(uintptr_t location, int offset) :
        location(location),
        offset(offset),
        wrapperCode(0x100, 0xcc)
    {
    }
    ~DetourHook() override
    {
        DWORD dwOldProt;
        if (VirtualProtect((LPVOID)location, offset, PAGE_EXECUTE_READWRITE, &dwOldProt))
        {
            memcpy((void*)location, originalCode, offset);
            VirtualProtect((LPVOID)location, offset, dwOldProt, &dwOldProt);
        }

        // In case the hook is currently executing, wait for it to end before releasing the wrapper code.
        std::lock_guard<std::mutex> lock(mutex);

        // BUG: There is a slight chance that the execution flow will enter the hook again and
        // will crash when trying to return because the wrapper code is gone.
    }

    uintptr_t getLocation() const override
    {
        return location;
    }

    uintptr_t location;
    int offset;
    uintptr_t ipBackup = 0;
    unsigned char *originalCode = nullptr;
    hl::code_vector wrapperCode;
    Hooker::HookCallback_t cbHook;
    std::mutex mutex;
};


static void JMPHookLocker(DetourHook *pHook, CpuContext *ctx)
{
    std::lock_guard<std::mutex> lock(pHook->mutex);

    pHook->cbHook(ctx);
}


const IHook *Hooker::hookVT(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize)
{
    // Check for invalid parameters.
    if (!classInstance || functionIndex < 0 || functionIndex >= vtBackupSize || !cbHook)
        return nullptr;

    auto pHook = std::make_unique<VTHook>(classInstance, functionIndex);

    // Replace the VT if it has not been done yet by a previous hook.
    auto itVT = m_fakeVTs.find(classInstance);
    if (itVT == m_fakeVTs.end())
    {
        // Copy the original VT.
        m_fakeVTs[classInstance].resize(vtBackupSize);
        for (int i = 0; i < vtBackupSize; i++)
        {
            m_fakeVTs[classInstance][i] = pHook->pOrgVT[i];
        }

        // Replace the VT pointer in the object instance.
        *pHook->pVT = m_fakeVTs[classInstance].data();
    }

    // Overwrite the hooked function in VT. This applies the hook.
    m_fakeVTs[classInstance][functionIndex] = cbHook;

    auto result = pHook.get();
    m_hooks.push_back(std::move(pHook));
    return result;
}


const IHook *Hooker::hookJMP(uintptr_t location, int nextInstructionOffset, void(*cbHook)())
{
    // Check for invalid parameters.
    if (!location || nextInstructionOffset < 5 || !cbHook)
        return nullptr;

    auto pHook = std::make_unique<JMPHook>(location, nextInstructionOffset);

    // Calculate delta.
    uintptr_t jmpFromLocToHook = (uintptr_t)cbHook - location - 5;

    // Generate patch.
    std::vector<unsigned char> jmpPatch(nextInstructionOffset, 0xcc);
    jmpPatch[0] = 0xe9; // JMP cbHook
    *(uintptr_t*)&jmpPatch[1] = jmpFromLocToHook;
    for (int i = 5; i < nextInstructionOffset; i++)
    {
        jmpPatch[i] = 0x90; // NOP
    }

    // Apply the hook by writing the jump.
    DWORD dwOldProt;
    if (!VirtualProtect((LPVOID)location, nextInstructionOffset, PAGE_EXECUTE_READWRITE, &dwOldProt))
        return nullptr;

    memcpy((void*)location, jmpPatch.data(), nextInstructionOffset);
    VirtualProtect((LPVOID)location, nextInstructionOffset, dwOldProt, &dwOldProt);

    auto result = pHook.get();
    m_hooks.push_back(std::move(pHook));
    return result;
}


const IHook *Hooker::hookDetour(uintptr_t location, int nextInstructionOffset, HookCallback_t cbHook)
{
    // Check for invalid parameters.
    if (!location || nextInstructionOffset < 5 || !cbHook)
        return nullptr;

    auto pHook = std::make_unique<DetourHook>(location, nextInstructionOffset);

    pHook->cbHook = cbHook;

    // Calculate deltas.
    uintptr_t jmpFromLocToWrapper = (uintptr_t)pHook->wrapperCode.data() - location - 5;
    uintptr_t callFromWrapperToLocker = (uintptr_t)JMPHookLocker - (uintptr_t)pHook->wrapperCode.data() - 13 - 5;

    // Generate wrapper code.
    // Push context to the stack. General purpose, flags and instruction pointer.
    pHook->wrapperCode[0] = 0x60; // PUSHAD
    pHook->wrapperCode[1] = 0x9c; // PUSHFD
    pHook->wrapperCode[2] = 0x68; // PUSH jumpBack
    *(uintptr_t*)&pHook->wrapperCode[3] = location + nextInstructionOffset;
    // Push stack pointer to stack as second argument to the locker function. Pointing to the context.
    pHook->wrapperCode[7] = 0x54; // PUSH ESP
    // Push pHook instance pointer as first argument to the locker function.
    pHook->wrapperCode[8] = 0x68; // PUSH pHook
    *(uintptr_t*)&pHook->wrapperCode[9] = (uintptr_t)pHook.get();
    // Call the hook callback.
    pHook->wrapperCode[13] = 0xe8; // CALL cbHook
    *(uintptr_t*)&pHook->wrapperCode[14] = callFromWrapperToLocker;
    // Cleanup parameters from cdecl call.
    pHook->wrapperCode[18] = 0x58; // POP EAX
    pHook->wrapperCode[19] = 0x58; // POP EAX
    // Backup the instruction pointer that may have been overwritten by the callback.
    pHook->wrapperCode[20] = 0x8f; // POP [ipBackup]
    pHook->wrapperCode[21] = 0x05;
    *(uintptr_t**)&pHook->wrapperCode[22] = &pHook->ipBackup;
    // Restore general purpose and flags registers.
    pHook->wrapperCode[26] = 0x9d; // POPFD
    pHook->wrapperCode[27] = 0x61; // POPAD
    pHook->originalCode = pHook->wrapperCode.data() + 28;
    // Execute original overwritten code.
    memcpy(pHook->originalCode, (void*)location, nextInstructionOffset);
    // Jump to the backed up instruction pointer.
    pHook->originalCode[nextInstructionOffset] = 0xff; // JMP [ipBackup]
    pHook->originalCode[1 + nextInstructionOffset] = 0x25;
    *(uintptr_t**)&pHook->originalCode[2 + nextInstructionOffset] = &pHook->ipBackup;

    // Generate patch.
    std::vector<unsigned char> jmpPatch(nextInstructionOffset, 0xcc);
    jmpPatch[0] = 0xe9; // JMP wrapper
    *(uintptr_t*)&jmpPatch[1] = jmpFromLocToWrapper;
    for (int i = 5; i < nextInstructionOffset; i++)
    {
        jmpPatch[i] = 0x90; // NOP
    }

    // Apply the hook by writing the jump.
    DWORD dwOldProt;
    if (!VirtualProtect((LPVOID)location, nextInstructionOffset, PAGE_EXECUTE_READWRITE, &dwOldProt))
        return nullptr;

    memcpy((void*)location, jmpPatch.data(), nextInstructionOffset);
    VirtualProtect((LPVOID)location, nextInstructionOffset, dwOldProt, &dwOldProt);

    auto result = pHook.get();
    m_hooks.push_back(std::move(pHook));
    return result;
}


void Hooker::unhook(const IHook *pHook)
{
    auto cond = [&](const std::unique_ptr<IHook>& uptr) {
        return uptr.get() == pHook;
    };

    m_hooks.erase(std::remove_if(m_hooks.begin(), m_hooks.end(), cond), m_hooks.end());
}
