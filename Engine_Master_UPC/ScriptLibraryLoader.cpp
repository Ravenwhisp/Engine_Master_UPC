#include "Globals.h"
#include "ScriptLibraryLoader.h"

#include "PdbPatcher.h"
#include "ScriptFactory.h"

#include <cstdio>
#include <filesystem>

bool ScriptLibraryLoader::load()
{
    if (m_module != nullptr)
    {
        DEBUG_WARN("[ScriptLibraryLoader] GameScripts DLL is already loaded.");
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
            DEBUG_ERROR("[ScriptLibraryLoader] Failed to copy PDB to runtime path.");
            return false;
        }

        if (!PdbPatcher::patchRuntimeDllPdbPath(runtimeDllPath, runtimePdbPath))
        {
            DEBUG_ERROR("[ScriptLibraryLoader] Failed to patch runtime DLL PDB path.");
            return false;
        }
    }
    else
    {
        DEBUG_WARN("[ScriptLibraryLoader] Source PDB not found: %s", sourcePdbPath.c_str());
    }

    m_module = LoadLibraryA(runtimeDllPath.c_str());

    if (m_module == nullptr)
    {
        DEBUG_ERROR("[ScriptLibraryLoader] Failed to load %s", runtimeDllPath.c_str());
        return false;
    }

    m_loadedDllPath = runtimeDllPath;

    return true;
}

bool ScriptLibraryLoader::unload()
{
    if (m_module == nullptr)
    {
        return true;
    }

    ScriptFactory::clear();

    if (!FreeLibrary(m_module))
    {
        DEBUG_ERROR("[ScriptLibraryLoader] Failed to unload %s", m_loadedDllPath.c_str());
        return false;
    }

    m_module = nullptr;
    m_loadedDllPath.clear();

    return true;
}

void ScriptLibraryLoader::cleanRuntimeFiles()
{
    const std::filesystem::path outputDirectory = ".";

    try
    {
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(outputDirectory))
        {
            if (!isRuntimeFile(entry))
            {
                continue;
            }

            std::filesystem::remove(entry.path());
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        DEBUG_WARN("[ScriptLibraryLoader] Failed to clean runtime script files. Error: %s", e.what());
    }
}

unsigned int ScriptLibraryLoader::getNextReloadVersion()
{
    ++m_reloadVersion;
    return m_reloadVersion;
}

std::string ScriptLibraryLoader::buildRuntimeDllPath(unsigned int version) const
{
    return std::string(RUNTIME_DLL_PREFIX) + std::to_string(version) + ".dll";
}

std::string ScriptLibraryLoader::buildRuntimePdbPath(unsigned int version) const
{
    char buffer[16];
    sprintf_s(buffer, "GS_%08X.pdb", version);
    return buffer;
}

bool ScriptLibraryLoader::copyFileToRuntimePath(const std::string& sourcePath, const std::string& runtimePath) const
{
    try
    {
        std::filesystem::copy_file(sourcePath, runtimePath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        DEBUG_ERROR(
            "[ScriptLibraryLoader] Failed to copy %s to %s. Error: %s",
            sourcePath.c_str(),
            runtimePath.c_str(),
            e.what());

        return false;
    }

    return true;
}

bool ScriptLibraryLoader::isRuntimeFile(const std::filesystem::directory_entry& entry) const
{
    if (!entry.is_regular_file())
    {
        return false;
    }

    const std::string fileName = entry.path().filename().string();

    const bool isRuntimeDll = fileName.rfind(RUNTIME_DLL_PREFIX, 0) == 0 && entry.path().extension() == ".dll";

    const bool isRuntimePdb = fileName.rfind("GS_", 0) == 0 && entry.path().extension() == ".pdb";

    return isRuntimeDll || isRuntimePdb;
}
