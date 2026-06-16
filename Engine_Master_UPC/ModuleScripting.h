#pragma once

#include "Module.h"
#include "ScriptLibraryLoader.h"
#include "ScriptBuildSystem.h"

#include <string>
#include <rapidjson/document.h>
#include <vector>
#include <future>

class ScriptComponent;

enum class ScriptReloadState
{
    Idle,
    Building,
    Reloading,
    BuildFailed,
    ReloadFailed,
    Completed
};

class ModuleScripting : public Module
{
public:
    ModuleScripting();
    ~ModuleScripting() override = default;

    bool init() override;
    void update() override;
    bool cleanUp() override;

#pragma region ScriptRuntime
    bool requestBuildAndReloadGameScriptsDll();
    void updateScriptReload();

    void instantiateSceneScripts();
    void destroySceneScripts();
    void destroySceneStateMachineBehaviours();

    ScriptReloadState getScriptReloadState() const { return m_scriptReloadState; }
    bool isScriptReloadBusy() const;
    void clearScriptReloadResult();
#pragma endregion

#pragma region BuildSettings
    bool loadScriptBuildSettings();
    bool saveScriptBuildSettings() const;

    ScriptBuildSettings& getScriptBuildSettings() { return m_buildSettings; }
    const ScriptBuildSettings& getScriptBuildSettings() const { return m_buildSettings; }
#pragma endregion

private: 
    struct ScriptReloadInfo
    {
        ScriptComponent* component = nullptr;
        std::string scriptName;
        rapidjson::Document fields;
    };

private:
#pragma region DllLoading
    bool reloadGameScriptsDllAfterSuccessfulBuild();
#pragma endregion

#pragma region ScriptReloadData
    std::vector<ScriptReloadInfo> saveSceneScriptReloadInfo();
    void restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos);
#pragma endregion

private:
#pragma region Constants
    static constexpr const char* SCRIPT_BUILD_SETTINGS_FILE = "script_build_settings.ini";
#pragma endregion

    ScriptLibraryLoader m_scriptLibraryLoader;
    ScriptBuildSystem m_scriptBuildSystem;

    std::future<bool> m_scriptBuildFuture;
    ScriptReloadState m_scriptReloadState = ScriptReloadState::Idle;

    ScriptBuildSettings m_buildSettings;

};
