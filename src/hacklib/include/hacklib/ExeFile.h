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
// Represents an executable image in PE or EFI format.
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
        std::vector<uint8_t> data;
        std::string name;
        SectionType type = SectionType::Unknown;
    };

public:
    ExeFile();
    ~ExeFile();

    // Load a module in the current process memory.
    bool loadFromMem(uintptr_t moduleBase);
    // Load a file from the file system.
    bool loadFromFile(const std::string& path);

    bool hasRelocs() const;
    bool isReloc(uintptr_t rva) const;

    // Returns null if not found.
    uintptr_t getExport(const std::string& name) const;

    std::span<const Section> getSections() const { return m_sections; }

private:
    std::unique_ptr<class ExeFileImpl> m_impl;
    bool m_valid = false;
    std::vector<uintptr_t> m_relocs;
    std::unordered_map<std::string, uintptr_t> m_exports;
    std::vector<Section> m_sections;
};
}

#endif
