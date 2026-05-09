#include "Globals.h"
#include "ModuleScripts.h"

#include "ScriptFactory.h"

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

    const std::string dllPath = "GameScripts.dll";

    m_gameScriptsModule = LoadLibraryA(dllPath.c_str());

    if (m_gameScriptsModule == nullptr)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to load %s", dllPath.c_str());
        return false;
    }

    m_loadedDllPath = dllPath;

    DEBUG_LOG("[ModuleScripts] Loaded %s", dllPath.c_str());

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