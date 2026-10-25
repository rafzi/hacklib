#ifndef HACKLIB_EXEFILE_H
#define HACKLIB_EXEFILE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <span>
#include <cstdint>


namespace hl
{
// Represents an executable image in PE or ELF format. PE images can be read on any platform, ELF
// images on UNIX.
class ExeFile
{
public:
    enum class SectionType
    {
        Unknown,
        Code
    };
    struct Section
    {
        uintptr_t virtualAddress = 0;
        std::vector<uint8_t> data;
        std::string name;
        SectionType type = SectionType::Unknown;
    };

public:
    ExeFile();
    ~ExeFile();

    // Load an image from memory. Sections are read at their file offsets, so this expects the
    // file's contents, not a module mapped by the loader.
    bool loadFromMem(uintptr_t moduleBase);
    // Load a file from the file system.
    bool loadFromFile(const std::string& path);

    bool hasRelocs() const;
    bool isReloc(uintptr_t rva) const;

    // Returns null if not found.
    uintptr_t getExport(const std::string& name) const;

    std::span<const Section> getSections() const { return m_sections; }

    uintptr_t getEntryPoint() const { return m_entryPoint; }

private:
    // Fill the members below from an image of that format (ExeFile_PE.cpp, ExeFile_UNIX.cpp).
    bool loadPe(uintptr_t moduleBase);
    bool loadElf(uintptr_t moduleBase);

private:
    bool m_valid = false;
    std::vector<uintptr_t> m_relocs;
    std::unordered_map<std::string, uintptr_t> m_exports;
    std::vector<Section> m_sections;
    uintptr_t m_entryPoint = 0;
};
}

#endif
