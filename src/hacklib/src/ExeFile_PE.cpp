#include "hacklib/ExeFile.h"
#include "PeFormat.h"

// PE images, read from the file layout (sections at their raw data offsets). Uses no OS API, so
// it works on any platform.

#ifdef ARCH_64BIT
using NtHeaders = hl::pe::ImageNtHeaders64;
#else
using NtHeaders = hl::pe::ImageNtHeaders32;
#endif


static std::vector<uintptr_t> LoadRelocs(uintptr_t relocData)
{
    std::vector<uintptr_t> relocs;

    auto regionPtr = (const uint32_t*)relocData;
    auto relocPtr = (const uint16_t*)relocData;

    while (true)
    {
        uint32_t rvaRegion = *regionPtr;
        uint32_t regionSize = *(regionPtr + 1);

        if (!regionSize)
        {
            break;
        }

        regionPtr += regionSize / 4;
        relocPtr += 4;

        while (relocPtr < (const uint16_t*)regionPtr)
        {
            uint16_t relocLow = *relocPtr++;
            int type = relocLow >> 12;
            relocLow &= 0x0fff;

            if (type == hl::pe::kRelBasedHighLow)
            {
                uintptr_t reloc = rvaRegion | relocLow;
                relocs.push_back(reloc);
            }
            else if (type == hl::pe::kRelBasedDir64)
            {
                uintptr_t reloc = rvaRegion + relocLow;
                relocs.push_back(reloc);
            }
        }
    }

    return relocs;
}

static uintptr_t RvaToRawDataOffset(const std::vector<const hl::pe::ImageSectionHeader*>& sectionHeaders, uint32_t rva)
{
    for (auto sectionHeader : sectionHeaders)
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


bool hl::ExeFile::loadPe(uintptr_t moduleBase)
{
    // Load DOS header and verify "MZ" signature.
    auto dosHeader = (const pe::ImageDosHeader*)moduleBase;
    if (dosHeader->e_magic != pe::kDosSignature || dosHeader->e_lfanew > 0x200)
    {
        return false;
    }

    // Load PE header.
    auto peHeader = (const NtHeaders*)((uintptr_t)moduleBase + dosHeader->e_lfanew);
    if (peHeader->Signature != pe::kNtSignature)
    {
        return false;
    }

    // Check if image type matches.
#ifdef ARCH_64BIT
    if (peHeader->FileHeader.Machine != pe::kMachineAmd64 ||
        (peHeader->FileHeader.Characteristics & pe::kFile32BitMachine))
#else
    if (peHeader->FileHeader.Machine != pe::kMachineI386 ||
        !(peHeader->FileHeader.Characteristics & pe::kFile32BitMachine))
#endif
    {
        return false;
    }

    // Load section headers.
    std::vector<const pe::ImageSectionHeader*> sectionHeaders;
    int nSections = peHeader->FileHeader.NumberOfSections;
    for (int i = 0; i < nSections; i++)
    {
        auto sectionHeader = (const pe::ImageSectionHeader*)((uintptr_t)peHeader + sizeof(NtHeaders) +
                                                             i * sizeof(pe::ImageSectionHeader));
        sectionHeaders.push_back(sectionHeader);

        Section outSection;
        for (int i = 0; i < pe::kSizeOfShortName; i++)
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
        if (sectionHeader->Characteristics & pe::kScnCntCode)
        {
            outSection.type = SectionType::Code;
        }
        outSection.virtualAddress = peHeader->OptionalHeader.ImageBase + sectionHeader->VirtualAddress;
        m_sections.push_back(std::move(outSection));
    }

    // Load relocs.
    auto relocDir = &peHeader->OptionalHeader.DataDirectory[pe::kDirectoryEntryBaseReloc];
    if (relocDir->VirtualAddress != 0)
    {
        uintptr_t relocsOffset = RvaToRawDataOffset(sectionHeaders, relocDir->VirtualAddress);
        if (relocsOffset)
        {
            m_relocs = LoadRelocs(moduleBase + relocsOffset);
        }
    }

    m_entryPoint = peHeader->OptionalHeader.ImageBase + peHeader->OptionalHeader.AddressOfEntryPoint;

    return true;
}
