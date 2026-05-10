#include "Globals.h"
#include "ModuleScripts.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "ScriptComponent.h"
#include "ScriptFactory.h"

#include "PdbPatcher.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

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

        if (!PdbPatcher::patchRuntimeDllPdbPath(runtimeDllPath, runtimePdbPath))
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
