#include "hacklib/ExeFile.h"
#include "hacklib/PeFormat.h"

// PE images, read from the file layout (sections at their raw data offsets). Uses no OS API, so
// it works on any platform.

#ifdef ARCH_64BIT
using NtHeaders = hl::pe::ImageNtHeaders64;
using Thunk = uint64_t;
static constexpr Thunk kOrdinalFlag = hl::pe::kOrdinalFlag64;
#else
using NtHeaders = hl::pe::ImageNtHeaders32;
using Thunk = uint32_t;
static constexpr Thunk kOrdinalFlag = hl::pe::kOrdinalFlag32;
#endif


static hl::Protection SectionProtection(uint32_t characteristics)
{
    hl::Protection protection = hl::PROTECTION_NOACCESS;
    if (characteristics & hl::pe::kScnMemRead)
        protection |= hl::PROTECTION_READ;
    if (characteristics & hl::pe::kScnMemWrite)
        protection |= hl::PROTECTION_WRITE;
    if (characteristics & hl::pe::kScnMemExecute)
        protection |= hl::PROTECTION_EXECUTE;
    return protection;
}


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
        // Old linkers leave VirtualSize at 0; the raw size is the size in memory then.
        outSection.virtualSize = sectionHeader->VirtualSize ? sectionHeader->VirtualSize : sectionHeader->SizeOfRawData;
        outSection.protection = SectionProtection(sectionHeader->Characteristics);
        m_sections.push_back(std::move(outSection));
    }

    m_imageBase = peHeader->OptionalHeader.ImageBase;
    m_imageSize = peHeader->OptionalHeader.SizeOfImage;
    m_headers.assign((const uint8_t*)moduleBase, (const uint8_t*)moduleBase + peHeader->OptionalHeader.SizeOfHeaders);

    // File data at an RVA, or null if no section holds it.
    auto at = [&](uint32_t rva) -> const uint8_t*
    {
        uintptr_t offset = RvaToRawDataOffset(sectionHeaders, rva);
        return offset ? (const uint8_t*)(moduleBase + offset) : nullptr;
    };

    // Load imports. Each descriptor names a module; its lookup table holds names or ordinals,
    // and the import address table at FirstThunk receives the addresses. Old linkers leave the
    // lookup table out, so the address table holds the lookup data before binding.
    auto importDir = &peHeader->OptionalHeader.DataDirectory[pe::kDirectoryEntryImport];
    if (importDir->VirtualAddress != 0)
    {
        for (auto desc = (const pe::ImageImportDescriptor*)at(importDir->VirtualAddress); desc && desc->Name; desc++)
        {
            auto module = (const char*)at(desc->Name);
            auto lookup = (const Thunk*)at(desc->OriginalFirstThunk ? desc->OriginalFirstThunk : desc->FirstThunk);
            if (!module || !lookup)
            {
                break;
            }
            for (size_t i = 0; lookup[i] != 0; i++)
            {
                Import import;
                import.module = module;
                import.slot = m_imageBase + desc->FirstThunk + i * sizeof(Thunk);
                if (lookup[i] & kOrdinalFlag)
                {
                    import.ordinal = (uint16_t)(lookup[i] & 0xffff);
                }
                else if (auto hintName = at((uint32_t)lookup[i]))
                {
                    import.name = (const char*)hintName + 2; // after the 16-bit hint
                }
                m_imports.push_back(std::move(import));
            }
        }
    }

    // Load exports by name. A function RVA inside the export directory is a forwarder string
    // naming another module's export, not code in this image; those are left out.
    auto exportDir = &peHeader->OptionalHeader.DataDirectory[pe::kDirectoryEntryExport];
    auto exports = exportDir->VirtualAddress != 0 ? (const pe::ImageExportDirectory*)at(exportDir->VirtualAddress)
                                                  : nullptr;
    if (exports)
    {
        auto functions = (const uint32_t*)at(exports->AddressOfFunctions);
        auto names = (const uint32_t*)at(exports->AddressOfNames);
        auto ordinals = (const uint16_t*)at(exports->AddressOfNameOrdinals);
        for (uint32_t i = 0; functions && names && ordinals && i < exports->NumberOfNames; i++)
        {
            auto name = (const char*)at(names[i]);
            if (!name || ordinals[i] >= exports->NumberOfFunctions)
            {
                continue;
            }
            uint32_t rva = functions[ordinals[i]];
            if (rva >= exportDir->VirtualAddress && rva < exportDir->VirtualAddress + exportDir->Size)
            {
                continue;
            }
            m_exports[name] = m_imageBase + rva;
        }
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
