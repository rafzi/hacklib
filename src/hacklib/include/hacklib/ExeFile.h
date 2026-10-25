#ifndef HACKLIB_EXEFILE_H
#define HACKLIB_EXEFILE_H

#include "hacklib/Memory.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <span>
#include <cstdint>


namespace hl
{
// Represents an executable image in PE or ELF format. PE images can be read on any platform, ELF
// images on UNIX. Addresses are as linked: PE addresses assume the image at its preferred base.
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
        // Size in memory. Can exceed data, which the loader then pads with zeros.
        size_t virtualSize = 0;
        std::vector<uint8_t> data;
        std::string name;
        SectionType type = SectionType::Unknown;
        Protection protection = PROTECTION_NOACCESS;
    };
    // A function a PE image imports. The loader writes its address to the slot.
    struct Import
    {
        std::string module;
        // Empty if imported by ordinal.
        std::string name;
        uint16_t ordinal = 0;
        // Address of the import address table entry.
        uintptr_t slot = 0;
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
    // PE: the RVAs of the pointer-sized fixups.
    std::span<const uintptr_t> getRelocs() const { return m_relocs; }

    // Returns null if not found. PE: forwarded exports are not included.
    uintptr_t getExport(const std::string& name) const;
    const std::unordered_map<std::string, uintptr_t>& getExports() const { return m_exports; }

    std::span<const Import> getImports() const { return m_imports; }

    std::span<const Section> getSections() const { return m_sections; }

    uintptr_t getEntryPoint() const { return m_entryPoint; }

    // PE: the preferred base address, the size in memory and the headers as they are mapped.
    uintptr_t getImageBase() const { return m_imageBase; }
    size_t getImageSize() const { return m_imageSize; }
    std::span<const uint8_t> getHeaders() const { return m_headers; }

private:
    // Fill the members below from an image of that format (ExeFile_PE.cpp, ExeFile_UNIX.cpp).
    bool loadPe(uintptr_t moduleBase);
    bool loadElf(uintptr_t moduleBase);

private:
    bool m_valid = false;
    std::vector<uintptr_t> m_relocs;
    std::unordered_map<std::string, uintptr_t> m_exports;
    std::vector<Import> m_imports;
    std::vector<Section> m_sections;
    uintptr_t m_entryPoint = 0;
    uintptr_t m_imageBase = 0;
    size_t m_imageSize = 0;
    std::vector<uint8_t> m_headers;
};
}

#endif
