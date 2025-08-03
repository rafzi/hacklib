#include "hacklib/ExeFile.h"
#include <elf.h>
#include <cstring>


#ifdef ARCH_64BIT
using Elf_Ehdr = Elf64_Ehdr;
using Elf_Phdr = Elf64_Phdr;
using Elf_Shdr = Elf64_Shdr;
using Elf_Sym = Elf64_Sym;
using Elf_Rel = Elf64_Rel;
#else
using Elf_Ehdr = Elf32_Ehdr;
using Elf_Phdr = Elf32_Phdr;
using Elf_Shdr = Elf32_Shdr;
using Elf_Sym = Elf32_Sym;
using Elf_Rel = Elf32_Rel;
#endif


class hl::ExeFileImpl
{
public:
    Elf_Ehdr* elfHeader = nullptr;
    Elf_Shdr* sectionHeaders = nullptr;
    Elf_Shdr* strTableHeader = nullptr;
    char* strTable = nullptr;
};


static std::unordered_map<std::string, uintptr_t> LoadSymbols(Elf_Sym* symTable, size_t symTableNum,
                                                              const char* strTable)
{
    std::unordered_map<std::string, uintptr_t> symbols;

    for (size_t i = 0; i < symTableNum; i++)
    {
        auto sym = &symTable[i];
        const char* name = sym->st_name > 0 ? &strTable[sym->st_name] : "";
        if (name)
        {
            symbols[name] = (uintptr_t)sym->st_value;
        }
    }

    return symbols;
}


hl::ExeFile::ExeFile()
{
    m_impl = std::make_unique<ExeFileImpl>();
}

hl::ExeFile::~ExeFile() = default;


bool hl::ExeFile::loadFromMem(uintptr_t moduleBase)
{
    m_valid = false;

    m_impl->elfHeader = (Elf_Ehdr*)moduleBase;
    if (m_impl->elfHeader->e_ident[EI_MAG0] != ELFMAG0 || m_impl->elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
        m_impl->elfHeader->e_ident[EI_MAG2] != ELFMAG2 || m_impl->elfHeader->e_ident[EI_MAG3] != ELFMAG3)
    {
        return false;
    }

    if (m_impl->elfHeader->e_ident[EI_VERSION] != EV_CURRENT)
    {
        return false;
    }

#ifdef ARCH_64BIT
    if (m_impl->elfHeader->e_ident[EI_CLASS] != ELFCLASS64 && m_impl->elfHeader->e_machine != EM_X86_64)
#else
    if (m_impl->elfHeader->e_ident[EI_CLASS] != ELFCLASS32 && m_impl->elfHeader->e_machine != EM_386)
#endif
    {
        return false;
    }

    if (m_impl->elfHeader->e_ident[EI_OSABI] != ELFOSABI_LINUX && m_impl->elfHeader->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        return false;
    }

    const size_t strTableSectionIndex = m_impl->elfHeader->e_shstrndx;
    m_impl->sectionHeaders = (Elf_Shdr*)(moduleBase + m_impl->elfHeader->e_shoff);
    m_impl->strTableHeader = &m_impl->sectionHeaders[strTableSectionIndex];
    m_impl->strTable = (char*)(moduleBase + m_impl->strTableHeader->sh_offset);

    const size_t numSections = m_impl->elfHeader->e_shnum;
    size_t symTableSectionIndex = -1;
    for (size_t i = 0; i < numSections; i++)
    {
        auto section = &m_impl->sectionHeaders[i];
        auto sectionData = (uint8_t*)(moduleBase + section->sh_offset);
        auto sectionName = m_impl->strTable + section->sh_name;

        Section outSection;
        outSection.name = sectionName;
        if (section->sh_type != SHT_NOBITS)
        {
            outSection.data.assign(sectionData, sectionData + section->sh_size);
        }

        switch (section->sh_type)
        {
        case SHT_PROGBITS:
            if (outSection.name == ".text")
            {
                outSection.type = SectionType::Code;
            }
            break;

        case SHT_SYMTAB:
        case SHT_DYNSYM:
            symTableSectionIndex = i;
            break;

        case SHT_STRTAB:
            if (i != strTableSectionIndex && symTableSectionIndex != (size_t)-1)
            {
                char* strTable = (char*)(moduleBase + m_impl->sectionHeaders[i].sh_offset);
                Elf_Shdr* symTableSectionHeader = &m_impl->sectionHeaders[symTableSectionIndex];
                auto symTable = (Elf_Sym*)(moduleBase + symTableSectionHeader->sh_offset);
                const size_t symTableNum = symTableSectionHeader->sh_size / symTableSectionHeader->sh_entsize;
                auto symbols = LoadSymbols(symTable, symTableNum, strTable);
                m_exports.insert(symbols.begin(), symbols.end());
                symTableSectionIndex = -1;
            }

        case SHT_REL:
        {
            size_t numRelocs = section->sh_size / sizeof(Elf_Rel);
            for (size_t r = 0; r < numRelocs; r++)
            {
                Elf_Rel* rel = ((Elf_Rel*)sectionData) + r;
                m_relocs.push_back(rel->r_offset);
            }
            break;
        }

        default:
            break;
        }

        m_sections.push_back(std::move(outSection));
    }

    m_valid = true;
    return true;
}
