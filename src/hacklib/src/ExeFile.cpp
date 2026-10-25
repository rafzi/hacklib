#include "hacklib/ExeFile.h"
#include <algorithm>
#include <fstream>


hl::ExeFile::ExeFile() = default;

hl::ExeFile::~ExeFile() = default;


bool hl::ExeFile::loadFromMem(uintptr_t moduleBase)
{
    m_valid = false;
    m_relocs.clear();
    m_exports.clear();
    m_imports.clear();
    m_sections.clear();
    m_entryPoint = 0;
    m_imageBase = 0;
    m_imageSize = 0;
    m_headers.clear();

    // The format decides the parser, not the platform.
    auto magic = (const uint8_t*)moduleBase;
    if (magic[0] == 'M' && magic[1] == 'Z')
    {
        m_valid = loadPe(moduleBase);
    }
    else if (magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F')
    {
        m_valid = loadElf(moduleBase);
    }
    return m_valid;
}

bool hl::ExeFile::loadFromFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const size_t size = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);
    if (size == (size_t)-1)
    {
        return false;
    }

    std::vector<char> buf(size);
    if (file.read(buf.data(), static_cast<std::streamsize>(size)))
    {
        return loadFromMem((uintptr_t)buf.data());
    }
    return false;
}

bool hl::ExeFile::hasRelocs() const
{
    if (!m_valid)
    {
        throw std::runtime_error("no exe file loaded");
    }

    return !m_relocs.empty();
}

bool hl::ExeFile::isReloc(uintptr_t rva) const
{
    if (!m_valid)
    {
        throw std::runtime_error("no exe file loaded");
    }

    auto it = std::ranges::find(m_relocs, rva);
    return it != m_relocs.end();
}

uintptr_t hl::ExeFile::getExport(const std::string& name) const
{
    auto it = m_exports.find(name);
    if (it != m_exports.end())
    {
        return it->second;
    }

    return 0;
}
