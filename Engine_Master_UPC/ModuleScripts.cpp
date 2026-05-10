#include "Globals.h"
#include "ModuleScripts.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "ScriptComponent.h"
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

    const std::string sourceDllPath = SCRIPT_DLL_NAME;
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

bool ModuleScripts::reloadGameScriptsDll()
{
    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        return false;
    }

    std::vector<ScriptReloadInfo> reloadInfo = saveSceneScriptReloadInfo();

    destroySceneScripts();

    if (!unloadGameScriptsDll())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to unload GameScripts DLL during reload.");
        return false;
    }

    if (!loadGameScriptsDll())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to load GameScripts DLL during reload.");
        return false;
    }

    instantiateSceneScripts();

    restoreSceneScriptReloadInfo(reloadInfo);
    app->getModuleScene()->getScene()->fixSceneReferences();

    DEBUG_LOG("[ModuleScripts] GameScripts DLL reloaded successfully.");

    return true;
}

bool ModuleScripts::buildGameScriptsProject()
{
    return false;
}

bool ModuleScripts::buildAndReloadGameScriptsDll()
{
    return false;
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

std::string ModuleScripts::buildRuntimeDllPath()
{
    ++m_reloadVersion;

    return std::string(RUNTIME_DLL_PREFIX) + std::to_string(m_reloadVersion) + ".dll";
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
