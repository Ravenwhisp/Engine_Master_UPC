#include "Globals.h"
#include "PdbPatcher.h"

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{
    constexpr uint32_t RSDS_SIGNATURE = 0x53445352; // "RSDS" in little endian

    bool rvaToFileOffset(
        DWORD rva,
        const IMAGE_NT_HEADERS* ntHeaders,
        const IMAGE_SECTION_HEADER* sections,
        DWORD& outOffset)
    {
        for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i)
        {
            const IMAGE_SECTION_HEADER& section = sections[i];

            const DWORD sectionStart = section.VirtualAddress;
            const DWORD sectionSize = std::max(section.Misc.VirtualSize, section.SizeOfRawData);
            const DWORD sectionEnd = sectionStart + sectionSize;

            if (rva >= sectionStart && rva < sectionEnd)
            {
                outOffset = section.PointerToRawData + (rva - sectionStart);
                return true;
            }
        }

        return false;
    }
}

bool PdbPatcher::patchRuntimeDllPdbPath(const std::string& runtimeDllPath, const std::string& runtimePdbPath)
{
    std::fstream file(runtimeDllPath, std::ios::in | std::ios::out | std::ios::binary);

    if (!file.is_open())
    {
        DEBUG_ERROR("[PdbPatcher] Failed to open runtime DLL for PDB patching: %s", runtimeDllPath.c_str());
        return false;
    }

    std::vector<char> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (data.size() < sizeof(IMAGE_DOS_HEADER))
    {
        DEBUG_ERROR("[PdbPatcher] Runtime DLL is too small to contain a DOS header: %s", runtimeDllPath.c_str());
        return false;
    }

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(data.data());

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        DEBUG_ERROR("[PdbPatcher] Invalid DOS signature in runtime DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    if (dosHeader->e_lfanew <= 0 || static_cast<size_t>(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS) > data.size())
    {
        DEBUG_ERROR("[PdbPatcher] Invalid NT header offset in runtime DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(data.data() + dosHeader->e_lfanew);

    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        DEBUG_ERROR("[PdbPatcher] Invalid NT signature in runtime DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        DEBUG_ERROR("[PdbPatcher] Unsupported PE format. Expected PE32+ / x64 DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    const IMAGE_DATA_DIRECTORY& debugDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

    if (debugDirectory.VirtualAddress == 0 || debugDirectory.Size == 0)
    {
        DEBUG_WARN("[PdbPatcher] Runtime DLL has no debug directory: %s", runtimeDllPath.c_str());
        return false;
    }

    const IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(ntHeaders);

    DWORD debugDirectoryOffset = 0;
    if (!rvaToFileOffset(debugDirectory.VirtualAddress, ntHeaders, sections, debugDirectoryOffset))
    {
        DEBUG_ERROR("[PdbPatcher] Failed to convert debug directory RVA to file offset.");
        return false;
    }

    if (static_cast<size_t>(debugDirectoryOffset) + debugDirectory.Size > data.size())
    {
        DEBUG_ERROR("[PdbPatcher] Debug directory is outside runtime DLL file data.");
        return false;
    }

    const size_t debugEntryCount = debugDirectory.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    const std::string runtimePdbFileName =
        std::filesystem::path(runtimePdbPath).filename().string();

    for (size_t i = 0; i < debugEntryCount; ++i)
    {
        const size_t debugEntryOffset = static_cast<size_t>(debugDirectoryOffset) + i * sizeof(IMAGE_DEBUG_DIRECTORY);

        auto* debugEntry = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(data.data() + debugEntryOffset);

        if (debugEntry->Type != IMAGE_DEBUG_TYPE_CODEVIEW)
        {
            continue;
        }

        DWORD codeViewOffset = debugEntry->PointerToRawData;

        if (codeViewOffset == 0)
        {
            if (!rvaToFileOffset(debugEntry->AddressOfRawData, ntHeaders, sections, codeViewOffset))
            {
                DEBUG_ERROR("[PdbPatcher] Failed to convert CodeView RVA to file offset.");
                return false;
            }
        }

        if (debugEntry->SizeOfData < 24)
        {
            DEBUG_ERROR("[PdbPatcher] CodeView debug data is too small.");
            return false;
        }

        if (static_cast<size_t>(codeViewOffset) + debugEntry->SizeOfData > data.size())
        {
            DEBUG_ERROR("[PdbPatcher] CodeView debug data is outside runtime DLL file data.");
            return false;
        }

        char* codeViewData = data.data() + codeViewOffset;

        const uint32_t signature = *reinterpret_cast<uint32_t*>(codeViewData);

        if (signature != RSDS_SIGNATURE)
        {
            DEBUG_WARN("[PdbPatcher] CodeView debug data is not RSDS. Skipping.");
            continue;
        }

        // RSDS layout:
        // DWORD signature;
        // GUID guid;
        // DWORD age;
        // char pdbFileName[];
        char* pdbPath = codeViewData + 24;
        const size_t pdbPathCapacity = debugEntry->SizeOfData - 24;

        size_t oldLength = 0;
        while (oldLength < pdbPathCapacity && pdbPath[oldLength] != '\0')
        {
            ++oldLength;
        }

        if (oldLength == pdbPathCapacity)
        {
            DEBUG_ERROR("[PdbPatcher] Could not find null terminator in embedded PDB path.");
            return false;
        }

        if (runtimePdbFileName.size() > oldLength)
        {
            DEBUG_ERROR("[PdbPatcher] Runtime PDB name '%s' is longer than embedded PDB path '%s'. Cannot patch safely.", runtimePdbFileName.c_str(), pdbPath);

            return false;
        }

        std::memset(pdbPath, 0, oldLength);
        std::memcpy(pdbPath, runtimePdbFileName.c_str(), runtimePdbFileName.size());

        file.seekp(0, std::ios::beg);
        file.write(data.data(), data.size());
        file.close();

        return true;
    }

    DEBUG_ERROR("[PdbPatcher] No RSDS CodeView debug entry found in runtime DLL.");
    return false;
}