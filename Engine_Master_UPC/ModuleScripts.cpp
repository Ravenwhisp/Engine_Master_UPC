#include "Globals.h"
#include "ModuleScripts.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "ScriptComponent.h"
#include "ScriptFactory.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

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

ModuleScripts::ModuleScripts()
{
    m_buildSettings.projectPath = "C:\\ReposVS\\Engine_Master_UPC\\GameScripts\\GameScripts.vcxproj";
    m_buildSettings.solutionDir = "C:\\ReposVS\\Engine_Master_UPC\\Engine_Master_UPC\\";
}

bool ModuleScripts::init()
{
    return loadGameScriptsDll();
}

bool ModuleScripts::cleanUp()
{
    return unloadGameScriptsDll();
}

bool ModuleScripts::buildAndReloadGameScriptsDll()
{
    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        DEBUG_WARN("[ModuleScripts] Cannot build and reload scripts while playing.");
        return false;
    }

    DEBUG_LOG("[ModuleScripts] Building and reloading GameScripts DLL...");

    if (!buildGameScriptsProject())
    {
        DEBUG_ERROR("[ModuleScripts] GameScripts build failed. Current scripts remain loaded.");
        return false;
    }

    std::vector<ScriptReloadInfo> reloadInfos = saveSceneScriptReloadInfo();

    destroySceneScripts();

    if (!unloadGameScriptsDll())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to unload current GameScripts DLL.");
        return false;
    }

    if (!loadGameScriptsDll())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to load new GameScripts DLL after build.");
        return false;
    }

    instantiateSceneScripts();
    restoreSceneScriptReloadInfo(reloadInfos);
    app->getModuleScene()->getScene()->fixSceneReferences();

    DEBUG_LOG("[ModuleScripts] GameScripts build and reload completed successfully.");

    return true;
}

void ModuleScripts::instantiateSceneScripts()
{
    const std::vector<ScriptComponent*>& scriptComponents = app->getModuleScene()->getScriptComponents();

    for (ScriptComponent* scriptComponent : scriptComponents)
    {
        if (!scriptComponent || scriptComponent->getScriptName().empty())
        {
            continue;
        }

        if (!scriptComponent->getScript())
        {
            bool created = scriptComponent->createScriptInstance();

            if (!created)
            {
                DEBUG_ERROR("[ModuleScripts] Failed to create script: %s", scriptComponent->getScriptName().c_str());
                continue;
            }
        }

        scriptComponent->resetStartState();
    }
}

void ModuleScripts::destroySceneScripts()
{
    const std::vector<ScriptComponent*>& scriptComponents = app->getModuleScene()->getScriptComponents();

    for (ScriptComponent* scriptComponent : scriptComponents)
    {
        if (!scriptComponent)
        {
            continue;
        }

        scriptComponent->destroyScriptInstance();
    }
}

bool ModuleScripts::loadGameScriptsDll()
{
    if (m_gameScriptsModule != nullptr)
    {
        DEBUG_WARN("[ModuleScripts] GameScripts DLL is already loaded.");
        return true;
    }

    const unsigned int reloadVersion = getNextReloadVersion();

    const std::string sourceDllPath = SCRIPT_DLL_NAME;
    const std::string sourcePdbPath = SCRIPT_PDB_NAME;

    const std::string runtimeDllPath = buildRuntimeDllPath(reloadVersion);
    const std::string runtimePdbPath = buildRuntimePdbPath(reloadVersion);

    if (!copyFileToRuntimePath(sourceDllPath, runtimeDllPath))
    {
        return false;
    }

    if (std::filesystem::exists(sourcePdbPath))
    {
        if (!copyFileToRuntimePath(sourcePdbPath, runtimePdbPath))
        {
            DEBUG_ERROR("[ModuleScripts] Failed to copy PDB to runtime path.");
            return false;
        }

        if (!patchRuntimeDllPdbPath(runtimeDllPath, runtimePdbPath))
        {
            DEBUG_ERROR("[ModuleScripts] Failed to patch runtime DLL PDB path.");
            return false;
        }
    }
    else
    {
        DEBUG_WARN("[ModuleScripts] Source PDB not found: %s", sourcePdbPath.c_str());
    }

    m_gameScriptsModule = LoadLibraryA(runtimeDllPath.c_str());

    if (m_gameScriptsModule == nullptr)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to load %s", runtimeDllPath.c_str());
        return false;
    }

    m_loadedDllPath = runtimeDllPath;

    DEBUG_LOG("[ModuleScripts] Loaded %s", runtimeDllPath.c_str());

    return true;
}

bool ModuleScripts::unloadGameScriptsDll()
{
    if (m_gameScriptsModule == nullptr)
    {
        return true;
    }

    ScriptFactory::clear();

    if (!FreeLibrary(m_gameScriptsModule))
    {
        DEBUG_ERROR("[ModuleScripts] Failed to unload %s", m_loadedDllPath.c_str());
        return false;
    }

    DEBUG_LOG("[ModuleScripts] Unloaded %s", m_loadedDllPath.c_str());

    m_gameScriptsModule = nullptr;
    m_loadedDllPath.clear();

    return true;
}

bool ModuleScripts::buildGameScriptsProject()
{
    if (m_buildSettings.projectPath.empty())
    {
        DEBUG_ERROR("[ModuleScripts] Script project path is empty.");
        return false;
    }

    if (m_buildSettings.solutionDir.empty())
    {
        DEBUG_ERROR("[ModuleScripts] Script solution directory is empty.");
        return false;
    }

    const std::string msbuildExe =
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe";

    const std::string buildBatPath = "ScriptsBuild.bat";
    const std::string buildLogPath = "ScriptsBuild.log";

    {
        std::ofstream buildBat(buildBatPath);

        if (!buildBat.is_open())
        {
            DEBUG_ERROR("[ModuleScripts] Failed to create %s", buildBatPath.c_str());
            return false;
        }

        buildBat << "@echo off\n";
        buildBat << "\"" << msbuildExe << "\" "
            << "\"" << m_buildSettings.projectPath << "\" "
            << "/p:Configuration=" << SCRIPT_BUILD_CONFIGURATION << " "
            << "/p:Platform=" << SCRIPT_BUILD_PLATFORM << " "
            << "/p:SolutionDir=" << m_buildSettings.solutionDir << "\n";

        buildBat << "exit /b %ERRORLEVEL%\n";
    }

    const std::string command =
        "cmd /C " + buildBatPath + " > " + buildLogPath + " 2>&1";

    DEBUG_LOG("[ModuleScripts] Building script project: %s", m_buildSettings.projectPath.c_str());
    DEBUG_LOG("[ModuleScripts] Build command: %s", command.c_str());

    const int result = std::system(command.c_str());

    if (result != 0)
    {
        DEBUG_ERROR("[ModuleScripts] GameScripts build failed. Result code: %d", result);
        DEBUG_ERROR("[ModuleScripts] Build output written to %s", buildLogPath.c_str());
        return false;
    }

    DEBUG_LOG("[ModuleScripts] GameScripts build succeeded.");
    DEBUG_LOG("[ModuleScripts] Build output written to %s", buildLogPath.c_str());

    return true;
}

unsigned int ModuleScripts::getNextReloadVersion()
{
    ++m_reloadVersion;
    return m_reloadVersion;
}

std::string ModuleScripts::buildRuntimeDllPath(unsigned int version) const
{
    return std::string(RUNTIME_DLL_PREFIX) + std::to_string(version) + ".dll";
}

std::string ModuleScripts::buildRuntimePdbPath(unsigned int version) const
{
    char buffer[16];
    sprintf_s(buffer, "GS_%08X.pdb", version);
    return buffer;
}

bool ModuleScripts::copyFileToRuntimePath(const std::string& sourcePath, const std::string& runtimePath)
{
    try
    {
        std::filesystem::copy_file(sourcePath, runtimePath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to copy %s to %s. Error: %s", sourcePath.c_str(), runtimePath.c_str(), e.what());

        return false;
    }

    DEBUG_LOG("[ModuleScripts] Copied %s to %s", sourcePath.c_str(), runtimePath.c_str());

    return true;
}

bool ModuleScripts::patchRuntimeDllPdbPath(const std::string& runtimeDllPath, const std::string& runtimePdbPath)
{
    std::fstream file(runtimeDllPath, std::ios::in | std::ios::out | std::ios::binary);

    if (!file.is_open())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to open runtime DLL for PDB patching: %s", runtimeDllPath.c_str());
        return false;
    }

    std::vector<char> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    if (data.size() < sizeof(IMAGE_DOS_HEADER))
    {
        DEBUG_ERROR("[ModuleScripts] Runtime DLL is too small to contain a DOS header: %s", runtimeDllPath.c_str());
        return false;
    }

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(data.data());

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        DEBUG_ERROR("[ModuleScripts] Invalid DOS signature in runtime DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    if (dosHeader->e_lfanew <= 0 || static_cast<size_t>(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS) > data.size())
    {
        DEBUG_ERROR("[ModuleScripts] Invalid NT header offset in runtime DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(data.data() + dosHeader->e_lfanew);

    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        DEBUG_ERROR("[ModuleScripts] Invalid NT signature in runtime DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    if (ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        DEBUG_ERROR("[ModuleScripts] Unsupported PE format. Expected PE32+ / x64 DLL: %s", runtimeDllPath.c_str());
        return false;
    }

    const IMAGE_DATA_DIRECTORY& debugDirectory =
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

    if (debugDirectory.VirtualAddress == 0 || debugDirectory.Size == 0)
    {
        DEBUG_WARN("[ModuleScripts] Runtime DLL has no debug directory: %s", runtimeDllPath.c_str());
        return false;
    }

    const IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(ntHeaders);

    DWORD debugDirectoryOffset = 0;
    if (!rvaToFileOffset(debugDirectory.VirtualAddress, ntHeaders, sections, debugDirectoryOffset))
    {
        DEBUG_ERROR("[ModuleScripts] Failed to convert debug directory RVA to file offset.");
        return false;
    }

    if (static_cast<size_t>(debugDirectoryOffset) + debugDirectory.Size > data.size())
    {
        DEBUG_ERROR("[ModuleScripts] Debug directory is outside runtime DLL file data.");
        return false;
    }

    const size_t debugEntryCount = debugDirectory.Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    const std::string runtimePdbFileName =
        std::filesystem::path(runtimePdbPath).filename().string();

    for (size_t i = 0; i < debugEntryCount; ++i)
    {
        const size_t debugEntryOffset =
            static_cast<size_t>(debugDirectoryOffset) + i * sizeof(IMAGE_DEBUG_DIRECTORY);

        auto* debugEntry =
            reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(data.data() + debugEntryOffset);

        if (debugEntry->Type != IMAGE_DEBUG_TYPE_CODEVIEW)
        {
            continue;
        }

        DWORD codeViewOffset = debugEntry->PointerToRawData;

        if (codeViewOffset == 0)
        {
            if (!rvaToFileOffset(debugEntry->AddressOfRawData, ntHeaders, sections, codeViewOffset))
            {
                DEBUG_ERROR("[ModuleScripts] Failed to convert CodeView RVA to file offset.");
                return false;
            }
        }

        if (debugEntry->SizeOfData < 24)
        {
            DEBUG_ERROR("[ModuleScripts] CodeView debug data is too small.");
            return false;
        }

        if (static_cast<size_t>(codeViewOffset) + debugEntry->SizeOfData > data.size())
        {
            DEBUG_ERROR("[ModuleScripts] CodeView debug data is outside runtime DLL file data.");
            return false;
        }

        char* codeViewData = data.data() + codeViewOffset;

        const uint32_t signature = *reinterpret_cast<uint32_t*>(codeViewData);

        if (signature != RSDS_SIGNATURE)
        {
            DEBUG_WARN("[ModuleScripts] CodeView debug data is not RSDS. Skipping.");
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
            DEBUG_ERROR("[ModuleScripts] Could not find null terminator in embedded PDB path.");
            return false;
        }

        if (runtimePdbFileName.size() > oldLength)
        {
            DEBUG_ERROR(
                "[ModuleScripts] Runtime PDB name '%s' is longer than embedded PDB path '%s'. Cannot patch safely.",
                runtimePdbFileName.c_str(),
                pdbPath);

            return false;
        }

        DEBUG_LOG("[ModuleScripts] Patching DLL PDB path from '%s' to '%s'",
            pdbPath,
            runtimePdbFileName.c_str());

        std::memset(pdbPath, 0, oldLength);
        std::memcpy(pdbPath, runtimePdbFileName.c_str(), runtimePdbFileName.size());

        file.seekp(0, std::ios::beg);
        file.write(data.data(), data.size());
        file.close();

        DEBUG_LOG("[ModuleScripts] Runtime DLL PDB path patched successfully.");

        return true;
    }

    DEBUG_ERROR("[ModuleScripts] No RSDS CodeView debug entry found in runtime DLL.");
    return false;
}

std::vector<ModuleScripts::ScriptReloadInfo> ModuleScripts::saveSceneScriptReloadInfo()
{
    const std::vector<ScriptComponent*>& scriptComponents = app->getModuleScene()->getScriptComponents();

    std::vector<ScriptReloadInfo> reloadInfo;
    reloadInfo.reserve(scriptComponents.size());

    for (ScriptComponent* scriptComponent : scriptComponents)
    {
        if (!scriptComponent || scriptComponent->getScriptName().empty())
        {
            continue;
        }

        ScriptReloadInfo info;
        info.component = scriptComponent;
        info.scriptName = scriptComponent->getScriptName();
        info.fields.SetObject();

        rapidjson::Value fieldsJson = scriptComponent->serializeScriptFieldsForReload(info.fields);
        info.fields.Swap(fieldsJson);

        reloadInfo.push_back(std::move(info));
    }

    return reloadInfo;
}

void ModuleScripts::restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos)
{
    for (ScriptReloadInfo& info : reloadInfos)
    {
        if (!info.component)
        {
            continue;
        }

        info.component->setScriptName(info.scriptName);

        if (!info.component->getScript())
        {
            DEBUG_ERROR("[ModuleScripts] Cannot restore fields because script was not recreated: %s", info.scriptName.c_str());
            continue;
        }

        info.component->deserializeScriptFieldsForReload(info.fields);
    }
}
