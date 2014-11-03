#include "hacklib/Hooker.h"
#include <Windows.h>
#include <algorithm>


using namespace hl;
using std::uintptr_t;


VTHook::VTHook(int vtSize) :
    fakeVT(vtSize, 0)
{
    pVT = nullptr;
    pOrgVT = nullptr;
}
VTHook::~VTHook()
{
    if (pVT)
        *pVT = pOrgVT;
}
uintptr_t VTHook::getOrgFunc(int functionIndex) const
{
    return pOrgVT[functionIndex];
}


JMPHook::JMPHook(int offset) :
    opcodeDetour(offset + 5, 0xcc)
{
    location = 0;
    this->offset = offset;
}
JMPHook::~JMPHook()
{
    DWORD dwOldProt;
    if (VirtualProtect((LPVOID)location, offset, PAGE_EXECUTE_READWRITE, &dwOldProt))
    {
        memcpy((void*)location, opcodeDetour.data(), offset);
        VirtualProtect((LPVOID)location, offset, dwOldProt, &dwOldProt);
    }
}
uintptr_t JMPHook::getLocation() const
{
    return location;
}
int JMPHook::getOffset() const
{
    return offset;
}



const VTHook *Hooker::hookVT(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize)
{
    // invalid parameters
    if (!classInstance || functionIndex < 0 || functionIndex >= vtBackupSize || !cbHook)
        return nullptr;

    // look up if instance was already hooked by this class
    // overwrite function in faked virtual table to prevent losing old hook
    for (auto& i : m_VTHooks)
    {
        if (i->pVT == (uintptr_t**)classInstance) {
            // apply hook
            i->fakeVT[functionIndex] = cbHook;

            return i.get();
        }
    }

    // setup hook class
    auto pHook = std::make_unique<VTHook>(vtBackupSize);
    pHook->pVT = (uintptr_t**)classInstance;
    pHook->pOrgVT = *pHook->pVT;
    for (int i = 0; i < vtBackupSize; i++)
    {
        pHook->fakeVT[i] = pHook->pOrgVT[i];
    }

    // overwrite function
    pHook->fakeVT[functionIndex] = cbHook;

    // apply hook
    *pHook->pVT = pHook->fakeVT.data();

    auto result = pHook.get();
    m_VTHooks.push_back(std::move(pHook));
    return result;
}

void Hooker::unhookVT(const VTHook *pHook)
{
    auto cond = [&](const std::unique_ptr<VTHook>& uptr) {
        return uptr.get() == pHook;
    };

    m_VTHooks.erase(remove_if(m_VTHooks.begin(), m_VTHooks.end(), cond), m_VTHooks.end());
}


const JMPHook *Hooker::hookJMP(uintptr_t location, int nextInstructionOffset, uintptr_t cbHook, uintptr_t *jmpBackAdr)
{
    // invalid parameters
    if (!location || nextInstructionOffset < 5 || !cbHook)
        return nullptr;

    // setup hook class
    auto pHook = std::make_unique<JMPHook>(nextInstructionOffset);
    pHook->location = location;
    pHook->offset = nextInstructionOffset;

    // calculate jump deltas
    uintptr_t jmpFromLocToHook = cbHook - location - 5;
    uintptr_t jmpFromDetourToLoc = location - (uintptr_t)pHook->opcodeDetour.data() - 5;

    memcpy(pHook->opcodeDetour.data(), (void*)location, nextInstructionOffset);
    pHook->opcodeDetour[nextInstructionOffset] = 0xe9;
    memcpy(pHook->opcodeDetour.data()+nextInstructionOffset+1, &jmpFromDetourToLoc, sizeof(uintptr_t));

    // write jmp back address
    *jmpBackAdr = (uintptr_t)pHook->opcodeDetour.data();

    // prepare patch
    auto pBuf = std::make_unique<unsigned char[]>(nextInstructionOffset);
    pBuf[0] = 0xe9;
    memcpy(&pBuf[1], &jmpFromLocToHook, 4);
    memset(&pBuf[5], 0x90, nextInstructionOffset-5);

    DWORD dwOldProt;
    // make the detour executable
    if (!VirtualProtect(pHook->opcodeDetour.data(), nextInstructionOffset+5, PAGE_EXECUTE_READWRITE, &dwOldProt))
        return nullptr;

    // apply hook
    if (VirtualProtect((LPVOID)location, nextInstructionOffset, PAGE_EXECUTE_READWRITE, &dwOldProt))
    {
        memcpy((void*)location, pBuf.get(), nextInstructionOffset);
        VirtualProtect((LPVOID)location, nextInstructionOffset, dwOldProt, &dwOldProt);
    } else {
        return nullptr;
    }

    auto result = pHook.get();
    m_JMPHooks.push_back(std::move(pHook));
    return result;
}

void Hooker::unhookJMP(const JMPHook *pHook)
{
    auto cond = [&](const std::unique_ptr<JMPHook>& uptr) {
        return uptr.get() == pHook;
    };

    m_JMPHooks.erase(remove_if(m_JMPHooks.begin(), m_JMPHooks.end(), cond), m_JMPHooks.end());
}