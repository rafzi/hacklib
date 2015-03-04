#include "hacklib/Hooker.h"
#include "hacklib/PageAllocator.h"
#include <algorithm>
#include <mutex>
#include <map>
#include <Windows.h>


using namespace hl;


struct FakeVT
{
    FakeVT(uintptr_t **instance, int vtBackupSize) :
        m_data(vtBackupSize),
        m_orgVT(*instance),
        m_refs(1)
    {
        // Copy original VT.
        for (int i = 0; i < vtBackupSize; i++)
        {
            m_data[i] = m_orgVT[i];
        }
    }
    hl::data_page_vector<uintptr_t> m_data;
    uintptr_t *m_orgVT;
    int m_refs;
};

class VTHookManager
{
public:
    uintptr_t getOrgFunc(uintptr_t **instance, int functionIndex)
    {
        return m_fakeVTs[instance]->m_orgVT[functionIndex];
    }
    void addHook(uintptr_t **instance, int functionIndex, uintptr_t cbHook, int vtBackupSize)
    {
        auto& fakeVT = m_fakeVTs[instance];
        if (fakeVT)
        {
            // The VT of this object was already hooked. Make the fake VT writable again.
            hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ|PROTECTION_WRITE);

            fakeVT->m_refs++;
        }
        else
        {
            // Create new fake VT (mirroring the original one).
            fakeVT = std::make_unique<FakeVT>(instance, vtBackupSize);

            // Replace the VT pointer in the object instance.
            *instance = fakeVT->m_data.data();
        }

        // Overwrite the hooked function in VT. This applies the hook.
        fakeVT->m_data[functionIndex] = cbHook;

        // Make the fake VT read-only like a real VT would be.
        hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ);
    }
    void removeHook(uintptr_t **instance, int functionIndex)
    {
        auto& fakeVT = m_fakeVTs[instance];
        if (fakeVT)
        {
            if (fakeVT->m_refs == 1)
            {
                // Last reference. Restore pointer to original VT.
                *instance = fakeVT->m_orgVT;

                m_fakeVTs.erase(instance);
            }
            else
            {
                // Keep using the fake VT, but restore the unhooked function pointer.
                hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ|PROTECTION_WRITE);
                fakeVT->m_data[functionIndex] = fakeVT->m_orgVT[functionIndex];
                hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ);

                fakeVT->m_refs--;
            }
        }
    }
private:
    std::map<uintptr_t**, std::unique_ptr<FakeVT>> m_fakeVTs;
};


static VTHookManager g_vtHookManager;


class VTHook : public IHook
{
public:
    VTHook(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize) :
        instance((uintptr_t**)classInstance),
        functionIndex(functionIndex)
    {
        g_vtHookManager.addHook(instance, functionIndex, cbHook, vtBackupSize);
    }
    ~VTHook() override
    {
        g_vtHookManager.removeHook(instance, functionIndex);
    }

    uintptr_t getLocation() const override
    {
        return g_vtHookManager.getOrgFunc(instance, functionIndex);
    }

    uintptr_t **instance;
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
    hl::code_page_vector wrapperCode;
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

    auto pHook = std::make_unique<VTHook>(classInstance, functionIndex, cbHook, vtBackupSize);

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
