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


static std::unordered_map<std::string, uintptr_t> LoadSymbols(const Elf_Sym* symTable, size_t symTableNum,
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


bool hl::ExeFile::loadElf(uintptr_t moduleBase)
{
    auto elfHeader = (const Elf_Ehdr*)moduleBase;
    if (elfHeader->e_ident[EI_MAG0] != ELFMAG0 || elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
        elfHeader->e_ident[EI_MAG2] != ELFMAG2 || elfHeader->e_ident[EI_MAG3] != ELFMAG3)
    {
        return false;
    }

    if (elfHeader->e_ident[EI_VERSION] != EV_CURRENT)
    {
        return false;
    }

#ifdef ARCH_64BIT
    if (elfHeader->e_ident[EI_CLASS] != ELFCLASS64 && elfHeader->e_machine != EM_X86_64)
#else
    if (elfHeader->e_ident[EI_CLASS] != ELFCLASS32 && elfHeader->e_machine != EM_386)
#endif
    {
        return false;
    }

    if (elfHeader->e_ident[EI_OSABI] != ELFOSABI_LINUX && elfHeader->e_ident[EI_OSABI] != ELFOSABI_SYSV)
    {
        return false;
    }

    const size_t strTableSectionIndex = elfHeader->e_shstrndx;
    auto sectionHeaders = (const Elf_Shdr*)(moduleBase + elfHeader->e_shoff);
    auto strTableHeader = &sectionHeaders[strTableSectionIndex];
    auto sectionNames = (const char*)(moduleBase + strTableHeader->sh_offset);

    const size_t numSections = elfHeader->e_shnum;
    size_t symTableSectionIndex = -1;
    for (size_t i = 0; i < numSections; i++)
    {
        auto section = &sectionHeaders[i];
        auto sectionData = (const uint8_t*)(moduleBase + section->sh_offset);
        auto sectionName = sectionNames + section->sh_name;

        Section outSection;
        outSection.name = sectionName;
        outSection.virtualAddress = section->sh_addr;
        outSection.virtualSize = section->sh_size;
        if (section->sh_flags & SHF_ALLOC)
            outSection.protection |= hl::PROTECTION_READ;
        if (section->sh_flags & SHF_WRITE)
            outSection.protection |= hl::PROTECTION_WRITE;
        if (section->sh_flags & SHF_EXECINSTR)
            outSection.protection |= hl::PROTECTION_EXECUTE;
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
                auto strTable = (const char*)(moduleBase + sectionHeaders[i].sh_offset);
                const Elf_Shdr* symTableSectionHeader = &sectionHeaders[symTableSectionIndex];
                auto symTable = (const Elf_Sym*)(moduleBase + symTableSectionHeader->sh_offset);
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
                auto rel = ((const Elf_Rel*)sectionData) + r;
                m_relocs.push_back(rel->r_offset);
            }
            break;
        }

        default:
            break;
        }

        m_sections.push_back(std::move(outSection));
    }

    m_entryPoint = elfHeader->e_entry;

    return true;
}
