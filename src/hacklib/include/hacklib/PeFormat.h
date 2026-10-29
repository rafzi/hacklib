#ifndef HACKLIB_PE_FORMAT_H
#define HACKLIB_PE_FORMAT_H

#include <cstddef>
#include <cstdint>

// PE/COFF structures as laid out in the file and in a mapped image. Defined here rather than taken
// from <windows.h> so that PE images can be parsed and mapped on any platform. Field names follow
// the PE specification (and winnt.h); the type names differ so that both can be included.

namespace hl::pe
{

constexpr uint16_t kDosSignature = 0x5A4D;    // "MZ"
constexpr uint32_t kNtSignature = 0x00004550; // "PE\0\0"
constexpr uint16_t kMachineI386 = 0x014C;
constexpr uint16_t kMachineAmd64 = 0x8664;
constexpr uint16_t kFileRelocsStripped = 0x0001;
constexpr uint16_t kFile32BitMachine = 0x0100;
constexpr uint32_t kScnCntCode = 0x00000020;
constexpr uint32_t kScnMemExecute = 0x20000000;
constexpr uint32_t kScnMemRead = 0x40000000;
constexpr uint32_t kScnMemWrite = 0x80000000;
constexpr int kDirectoryEntryExport = 0;
constexpr int kDirectoryEntryImport = 1;
constexpr int kDirectoryEntryException = 3;
constexpr int kDirectoryEntryBaseReloc = 5;
constexpr int kDirectoryEntryTls = 9;
constexpr int kNumberOfDirectoryEntries = 16;
constexpr int kSizeOfShortName = 8;
constexpr int kRelBasedAbsolute = 0;
constexpr int kRelBasedHighLow = 3;
constexpr int kRelBasedDir64 = 10;

struct ImageDosHeader
{
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    int32_t e_lfanew;
};

struct ImageFileHeader
{
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

struct ImageDataDirectory
{
    uint32_t VirtualAddress;
    uint32_t Size;
};

struct ImageOptionalHeader32
{
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    ImageDataDirectory DataDirectory[kNumberOfDirectoryEntries];
};

struct ImageOptionalHeader64
{
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    ImageDataDirectory DataDirectory[kNumberOfDirectoryEntries];
};

struct ImageNtHeaders32
{
    uint32_t Signature;
    ImageFileHeader FileHeader;
    ImageOptionalHeader32 OptionalHeader;
};

struct ImageNtHeaders64
{
    uint32_t Signature;
    ImageFileHeader FileHeader;
    ImageOptionalHeader64 OptionalHeader;
};

struct ImageSectionHeader
{
    uint8_t Name[kSizeOfShortName];
    uint32_t VirtualSize; // Misc.VirtualSize in winnt.h
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};

struct ImageImportDescriptor
{
    uint32_t OriginalFirstThunk; // RVA of the import lookup table (Characteristics in winnt.h)
    uint32_t TimeDateStamp;
    uint32_t ForwarderChain;
    uint32_t Name;
    uint32_t FirstThunk; // RVA of the import address table
};

struct ImageExportDirectory
{
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Name;
    uint32_t Base;
    uint32_t NumberOfFunctions;
    uint32_t NumberOfNames;
    uint32_t AddressOfFunctions;
    uint32_t AddressOfNames;
    uint32_t AddressOfNameOrdinals;
};

// Holds virtual addresses, not RVAs, so they are subject to relocation.
struct ImageTlsDirectory64
{
    uint64_t StartAddressOfRawData;
    uint64_t EndAddressOfRawData;
    uint64_t AddressOfIndex;
    uint64_t AddressOfCallBacks; // Null-terminated array of PIMAGE_TLS_CALLBACK
    uint32_t SizeOfZeroFill;
    uint32_t Characteristics;
};

// Header of a base relocation block, followed by 16-bit entries: type << 12 | offset in the page.
struct ImageBaseRelocation
{
    uint32_t VirtualAddress;
    uint32_t SizeOfBlock; // Including this header
};

// An x64 exception directory entry (RUNTIME_FUNCTION), sorted by BeginAddress. All RVAs.
struct ImageRuntimeFunctionEntry
{
    uint32_t BeginAddress;
    uint32_t EndAddress;
    uint32_t UnwindInfoAddress;
};

// What an import lookup table entry points to when importing by name.
struct ImageImportByName
{
    uint16_t Hint;
    char Name[1]; // Null-terminated
};

// Import lookup table entries are pointer-sized; the top bit marks an import by ordinal.
constexpr uint64_t kOrdinalFlag64 = 0x8000000000000000ULL;
constexpr uint32_t kOrdinalFlag32 = 0x80000000U;

// The natural layout matches the file format; checked here instead of relying on packing pragmas.
static_assert(sizeof(ImageDosHeader) == 64 && offsetof(ImageDosHeader, e_lfanew) == 0x3C);
static_assert(sizeof(ImageFileHeader) == 20);
static_assert(sizeof(ImageOptionalHeader32) == 224 && offsetof(ImageOptionalHeader32, ImageBase) == 28);
static_assert(sizeof(ImageOptionalHeader64) == 240 && offsetof(ImageOptionalHeader64, ImageBase) == 24);
static_assert(sizeof(ImageNtHeaders32) == 248 && offsetof(ImageNtHeaders32, OptionalHeader) == 24);
static_assert(sizeof(ImageNtHeaders64) == 264 && offsetof(ImageNtHeaders64, OptionalHeader) == 24);
static_assert(sizeof(ImageSectionHeader) == 40);
static_assert(sizeof(ImageImportDescriptor) == 20);
static_assert(sizeof(ImageExportDirectory) == 40);
static_assert(sizeof(ImageTlsDirectory64) == 40);
static_assert(sizeof(ImageBaseRelocation) == 8);
static_assert(sizeof(ImageRuntimeFunctionEntry) == 12);
static_assert(offsetof(ImageImportByName, Name) == 2);

}

#endif
