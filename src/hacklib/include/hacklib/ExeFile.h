#ifndef HACKLIB_EXEFILE_H
#define HACKLIB_EXEFILE_H

#include <vector>
#include <memory>


namespace hl {


// Represents an executable image in PE or EFI format.
class ExeFile
{
public:
    ExeFile();
    ~ExeFile();

    // Load a module in the current process memory.
    bool loadFromMem(uintptr_t moduleBase);

    bool hasRelocs() const;
    bool isReloc(uintptr_t rva) const;

private:
    std::unique_ptr<class ExeFileImpl> m_impl;

};

}

#endif