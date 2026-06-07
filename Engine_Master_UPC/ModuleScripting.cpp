#include "Globals.h"
#include "ModuleScripting.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "ScriptComponent.h"

#include <fstream>
#include <vector>
#include <chrono>
#include <future>

// Used to clean whitespace when parsing the script build settings .ini.
namespace
{
    std::string trim(const std::string& value)
    {
        const size_t first = value.find_first_not_of(" \t\r\n");

        if (first == std::string::npos)
        {
            return "";
        }

        const size_t last = value.find_last_not_of(" \t\r\n");

        return value.substr(first, last - first + 1);
    }
}

ModuleScripting::ModuleScripting()
{
}

bool ModuleScripting::init()
{
    loadScriptBuildSettings();

    m_scriptLibraryLoader.cleanRuntimeFiles();

    return m_scriptLibraryLoader.load();
}

void ModuleScripting::update()
{
    updateScriptReload();
}

bool ModuleScripting::cleanUp()
{
    // If the editor is closed while MSBuild is running, wait for the worker task to finish safely.
    if (m_scriptBuildFuture.valid())
    {
        m_scriptBuildFuture.wait();
    }

    const bool unloaded = m_scriptLibraryLoader.unload();

    m_scriptLibraryLoader.cleanRuntimeFiles();

    return unloaded;
}

bool ModuleScripting::requestBuildAndReloadGameScriptsDll()
{
    if (isScriptReloadBusy())
    {
        DEBUG_WARN("[ModuleScripting] GameScripts build/reload is already in progress.");
        return false;
    }

    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        DEBUG_WARN("[ModuleScripting] Cannot build and reload scripts while playing.");
        return false;
    }

    if (m_buildSettings.projectPath.empty())
    {
        DEBUG_ERROR("[ModuleScripting] Script project path is empty.");
        return false;
    }

    if (m_buildSettings.solutionDir.empty())
    {
        DEBUG_ERROR("[ModuleScripting] Script solution directory is empty.");
        return false;
    }

    const ScriptBuildSettings buildSettings = m_buildSettings;

    m_scriptReloadState = ScriptReloadState::Building;

    m_scriptBuildFuture = std::async(std::launch::async, [this, buildSettings]()
        {
            return m_scriptBuildSystem.build(buildSettings);
        });

    return true;
}

void ModuleScripting::updateScriptReload()
{
    if (m_scriptReloadState != ScriptReloadState::Building)
    {
        return;
    }

    if (!m_scriptBuildFuture.valid())
    {
        m_scriptReloadState = ScriptReloadState::BuildFailed;
        DEBUG_ERROR("[ModuleScripting] GameScripts build failed. Current scripts remain loaded.");
        return;
    }

    const std::future_status status = m_scriptBuildFuture.wait_for(std::chrono::seconds(0));

    if (status != std::future_status::ready)
    {
        return;
    }

    const bool buildSucceeded = m_scriptBuildFuture.get();

    if (!buildSucceeded)
    {
        m_scriptReloadState = ScriptReloadState::BuildFailed;
        DEBUG_ERROR("[ModuleScripting] GameScripts build failed. Current scripts remain loaded.");
        return;
    }

    m_scriptReloadState = ScriptReloadState::Reloading;

    // The build runs in a worker thread, but DLL reload and script restoration
    // must stay on the main thread because they touch scene/editor state.
    if (!reloadGameScriptsDllAfterSuccessfulBuild())
    {
        m_scriptReloadState = ScriptReloadState::ReloadFailed;
        DEBUG_ERROR("[ModuleScripting] GameScripts reload failed.");
        return;
    }

    m_scriptReloadState = ScriptReloadState::Completed;
    DEBUG_LOG("[ModuleScripting] GameScripts build and reload completed successfully.");
}

void ModuleScripting::instantiateSceneScripts()
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
                DEBUG_ERROR("[ModuleScripting] Failed to create script: %s", scriptComponent->getScriptName().c_str());
                continue;
            }
        }

        scriptComponent->resetStartState();
    }
}

void ModuleScripting::destroySceneScripts()
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

bool ModuleScripting::isScriptReloadBusy() const
{
    bool busy = false;

    if (m_scriptReloadState == ScriptReloadState::Building || m_scriptReloadState == ScriptReloadState::Reloading)
    {
        busy = true;
    }

    return busy;
}

void ModuleScripting::clearScriptReloadResult()
{
    if (m_scriptReloadState == ScriptReloadState::Completed || m_scriptReloadState == ScriptReloadState::BuildFailed || m_scriptReloadState == ScriptReloadState::ReloadFailed)
    {
        m_scriptReloadState = ScriptReloadState::Idle;
    }
}

bool ModuleScripting::loadScriptBuildSettings()
{
    std::ifstream file(SCRIPT_BUILD_SETTINGS_FILE);

    if (!file.is_open())
    {
        return false;
    }

    bool insideScriptBuildSection = false;

    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty())
        {
            continue;
        }

        if (line[0] == ';' || line[0] == '#')
        {
            continue;
        }

        if (line.front() == '[' && line.back() == ']')
        {
            const std::string sectionName = trim(line.substr(1, line.size() - 2));
            insideScriptBuildSection = sectionName == "ScriptBuild";
            continue;
        }

        if (!insideScriptBuildSection)
        {
            continue;
        }

        const size_t equalsPosition = line.find('=');

        if (equalsPosition == std::string::npos)
        {
            continue;
        }

        const std::string key = trim(line.substr(0, equalsPosition));
        const std::string value = trim(line.substr(equalsPosition + 1));

        if (key == "ProjectPath")
        {
            m_buildSettings.projectPath = value;
        }
        else if (key == "SolutionDir")
        {
            m_buildSettings.solutionDir = value;
        }
        else if (key == "Configuration")
        {
            m_buildSettings.configuration = value;
        }
        else if (key == "Platform")
        {
            m_buildSettings.platform = value;
        }
    }

    if (m_buildSettings.projectPath.empty())
    {
        DEBUG_WARN("[ModuleScripting] Script build ProjectPath is empty.");
        return false;
    }

    if (m_buildSettings.solutionDir.empty())
    {
        DEBUG_WARN("[ModuleScripting] Script build SolutionDir is empty.");
        return false;
    }

    if (m_buildSettings.configuration.empty())
    {
        DEBUG_WARN("[ModuleScripting] Script build Configuration is empty.");
        return false;
    }

    if (m_buildSettings.platform.empty())
    {
        DEBUG_WARN("[ModuleScripting] Script build Platform is empty.");
        return false;
    }

    return true;
}

bool ModuleScripting::saveScriptBuildSettings() const
{
    std::ofstream file(SCRIPT_BUILD_SETTINGS_FILE, std::ios::trunc);

    if (!file.is_open())
    {
        DEBUG_ERROR("[ModuleScripting] Failed to create script build settings file: %s", SCRIPT_BUILD_SETTINGS_FILE);
        return false;
    }

    file << "[ScriptBuild]\n";
    file << "ProjectPath=" << m_buildSettings.projectPath << "\n";
    file << "SolutionDir=" << m_buildSettings.solutionDir << "\n";
    file << "Configuration=" << m_buildSettings.configuration << "\n";
    file << "Platform=" << m_buildSettings.platform << "\n";

    file.close();

    DEBUG_LOG("[ModuleScripting] Script build settings saved to %s", SCRIPT_BUILD_SETTINGS_FILE);

    return true;
}

bool ModuleScripting::reloadGameScriptsDllAfterSuccessfulBuild()
{
    std::vector<ScriptReloadInfo> reloadInfos = saveSceneScriptReloadInfo();

    destroySceneScripts();

    if (!m_scriptLibraryLoader.unload())
    {
        DEBUG_ERROR("[ModuleScripting] Failed to unload current GameScripts DLL.");
        return false;
    }

    if (!m_scriptLibraryLoader.load())
    {
        DEBUG_ERROR("[ModuleScripting] Failed to load new GameScripts DLL after build.");
        return false;
    }

    instantiateSceneScripts();
    restoreSceneScriptReloadInfo(reloadInfos);
    app->getModuleScene()->getScene()->fixSceneReferences();

    return true;
}

std::vector<ModuleScripting::ScriptReloadInfo> ModuleScripting::saveSceneScriptReloadInfo()
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

void ModuleScripting::restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos)
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
            DEBUG_ERROR("[ModuleScripting] Cannot restore fields because script was not recreated: %s", info.scriptName.c_str());
            continue;
        }

        info.component->deserializeScriptFieldsForReload(info.fields);
    }
}
