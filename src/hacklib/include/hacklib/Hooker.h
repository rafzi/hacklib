#ifndef HACKLIB_HOOKER_H
#define HACKLIB_HOOKER_H

#include <cstdint>
#include <vector>
#include <memory>


namespace hl {


class VTHook
{
    friend class Hooker;
public:
    VTHook(int vtSize);
    ~VTHook();
    std::uintptr_t getOrgFunc(int functionIndex) const;
private:
    std::uintptr_t **pVT; // location of virtual table pointer
    std::uintptr_t *pOrgVT; // pointer to original virtual table
    std::vector<std::uintptr_t> fakeVT; // fake virtual table
};
class JMPHook
{
    friend class Hooker;
public:
    JMPHook(int offset);
    ~JMPHook();
    std::uintptr_t getLocation() const;
    int getOffset() const;
private:
    std::uintptr_t location;
    int offset;
    std::vector<unsigned char> opcodeDetour;
};

class Hooker
{
public:
    const VTHook *hookVT(std::uintptr_t classInstance, int functionIndex, std::uintptr_t cbHook, int vtBackupSize = 1024);
    void unhookVT(const VTHook *pHook);

    const JMPHook *hookJMP(std::uintptr_t location, int nextInstructionOffset, std::uintptr_t cbHook, std::uintptr_t *jmpBackAdr);
    void unhookJMP(const JMPHook *pHook);

private:
    std::vector<std::unique_ptr<VTHook>> m_VTHooks;
    std::vector<std::unique_ptr<JMPHook>> m_JMPHooks;

};

}

#endif
