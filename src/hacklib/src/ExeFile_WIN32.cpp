#include "hacklib/ExeFile.h"
#include <Windows.h>


class hl::ExeFileImpl
{
public:
    IMAGE_DOS_HEADER* dosHeader = nullptr;
    IMAGE_NT_HEADERS* peHeader = nullptr;
    std::vector<IMAGE_SECTION_HEADER*> sectionHeaders;
};


static std::vector<uintptr_t> LoadRelocs(uintptr_t relocData)
{
    std::vector<uintptr_t> relocs;

    DWORD* regionPtr = (DWORD*)relocData;
    WORD* relocPtr = (WORD*)relocData;

    while (true)
    {
        DWORD rvaRegion = *regionPtr;
        DWORD regionSize = *(regionPtr + 1);

        if (!regionSize)
        {
            break;
        }

        regionPtr += regionSize / 4;
        relocPtr += 4;

        while (relocPtr < (WORD*)regionPtr)
        {
            WORD relocLow = *relocPtr++;
            int type = relocLow >> 12;
            relocLow &= 0x0fff;

            if (type == IMAGE_REL_BASED_HIGHLOW)
            {
                uintptr_t reloc = rvaRegion | relocLow;
                relocs.push_back(reloc);
            }
            else if (type == IMAGE_REL_BASED_DIR64)
            {
                uintptr_t reloc = rvaRegion + relocLow;
                relocs.push_back(reloc);
            }
        }
    }

    return relocs;
}

static uintptr_t RvaToRawDataOffset(const hl::ExeFileImpl& impl, DWORD rva)
{
    for (auto sectionHeader : impl.sectionHeaders)
    {
        uint32_t va = sectionHeader->VirtualAddress;
        uint32_t size = sectionHeader->SizeOfRawData;
        if (rva >= va && rva < va + size)
        {
            uint32_t delta = rva - va;
            return sectionHeader->PointerToRawData + delta;
        }
    }
    return 0;
}


hl::ExeFile::ExeFile()
{
    m_impl = std::make_unique<ExeFileImpl>();
}

hl::ExeFile::~ExeFile() = default;


bool hl::ExeFile::loadFromMem(uintptr_t moduleBase)
{
    m_valid = false;

    // Load DOS header and verify "MZ" signature.
    m_impl->dosHeader = (IMAGE_DOS_HEADER*)moduleBase;
    if (m_impl->dosHeader->e_magic != IMAGE_DOS_SIGNATURE || m_impl->dosHeader->e_lfanew > 0x200)
    {
        return false;
    }

    // Load PE header.
    m_impl->peHeader = (IMAGE_NT_HEADERS*)((uintptr_t)moduleBase + m_impl->dosHeader->e_lfanew);
    if (m_impl->peHeader->Signature != IMAGE_NT_SIGNATURE)
    {
        return false;
    }

    // Check if image type matches.
#ifdef ARCH_64BIT
    if (m_impl->peHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        (m_impl->peHeader->FileHeader.Characteristics & IMAGE_FILE_32BIT_MACHINE))
#else
    if (m_impl->peHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        !(m_impl->peHeader->FileHeader.Characteristics & IMAGE_FILE_32BIT_MACHINE))
#endif
    {
        return false;
    }

    // Load section headers.
    int nSections = m_impl->peHeader->FileHeader.NumberOfSections;
    for (int i = 0; i < nSections; i++)
    {
        auto sectionHeader = (IMAGE_SECTION_HEADER*)((uintptr_t)m_impl->peHeader + sizeof(IMAGE_NT_HEADERS) +
                                                     i * sizeof(IMAGE_SECTION_HEADER));
        m_impl->sectionHeaders.push_back(sectionHeader);

        Section outSection;
        for (int i = 0; i < IMAGE_SIZEOF_SHORT_NAME; i++)
        {
            if (sectionHeader->Name[i] == 0)
            {
                break;
            }
            outSection.name.push_back((char)sectionHeader->Name[i]);
        }
        if (sectionHeader->SizeOfRawData > 0)
        {
            auto sectionData = (const uint8_t*)(moduleBase + sectionHeader->PointerToRawData);
            outSection.data.assign(sectionData, sectionData + sectionHeader->SizeOfRawData);
        }
        if (sectionHeader->Characteristics & IMAGE_SCN_CNT_CODE)
        {
            outSection.type = SectionType::Code;
        }
        m_sections.push_back(std::move(outSection));
    }

    // Load relocs.
    auto relocDir = &m_impl->peHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (relocDir->VirtualAddress != 0)
    {
        uintptr_t relocsOffset = RvaToRawDataOffset(*m_impl, relocDir->VirtualAddress);
        if (relocsOffset)
        {
            m_relocs = LoadRelocs(moduleBase + relocsOffset);
        }
    }

    m_valid = true;
    return true;
}
