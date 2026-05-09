#include "Globals.h"
#include "ModuleScripts.h"

#include "ScriptFactory.h"

#include <filesystem>

ModuleScripts::ModuleScripts()
{
}

bool ModuleScripts::init()
{
    return loadGameScriptsDll();
}

bool ModuleScripts::cleanUp()
{
    return unloadGameScriptsDll();
}

bool ModuleScripts::loadGameScriptsDll()
{
    if (m_gameScriptsModule != nullptr)
    {
        DEBUG_WARN("[ModuleScripts] GameScripts DLL is already loaded.");
        return true;
    }

    const std::string sourceDllPath = "GameScripts.dll";
    const std::string runtimeDllPath = buildRuntimeDllPath();

    if (!copySourceDllToRuntimeDll(sourceDllPath, runtimeDllPath))
    {
        return false;
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

std::string ModuleScripts::buildRuntimeDllPath()
{
    ++m_reloadVersion;

    return "GameScripts_runtime_" + std::to_string(m_reloadVersion) + ".dll";
}

bool ModuleScripts::copySourceDllToRuntimeDll(const std::string& sourceDllPath, const std::string& runtimeDllPath)
{
    try
    {
        std::filesystem::copy_file(sourceDllPath, runtimeDllPath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to copy %s to %s. Error: %s", sourceDllPath.c_str(), runtimeDllPath.c_str(), e.what());

        return false;
    }

    DEBUG_LOG("[ModuleScripts] Copied %s to %s", sourceDllPath.c_str(), runtimeDllPath.c_str());

    return true;
}