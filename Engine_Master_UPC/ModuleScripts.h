#pragma once

#include "Module.h"
#include "ScriptLibraryLoader.h"

#include <string>
#include <rapidjson/document.h>
#include <vector>
#include <filesystem>
#include <future>

class ScriptComponent;

struct ScriptBuildSettings
{
    std::string projectPath;
    std::string solutionDir;
};

enum class ScriptReloadState
{
    Idle,
    Building,
    Reloading,
    BuildFailed,
    ReloadFailed,
    Completed
};

class ModuleScripts : public Module
{
public:
    ModuleScripts();
    ~ModuleScripts() override = default;

    bool init() override;
    void update() override;
    bool cleanUp() override;

#pragma region ScriptRuntime
    bool requestBuildAndReloadGameScriptsDll();
    void updateScriptReload();

    void instantiateSceneScripts();
    void destroySceneScripts();

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

#pragma region ScriptBuild
    bool buildGameScriptsProject(const ScriptBuildSettings& buildSettings) const;
    std::filesystem::path resolveBuildPath(const std::string& path) const;
    bool validateScriptBuildPaths(const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir) const;
    
    bool runMsBuild(const std::filesystem::path& msbuildPath, const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir, const std::string& buildLogPath) const;
#pragma endregion

#pragma region ScriptReloadState
    std::vector<ScriptReloadInfo> saveSceneScriptReloadInfo();
    void restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos);
#pragma endregion

private:
#pragma region Constants
#if defined(_DEBUG)
    static constexpr const char* SCRIPT_BUILD_CONFIGURATION = "Debug";
#else
    static constexpr const char* SCRIPT_BUILD_CONFIGURATION = "Release";
#endif
    static constexpr const char* SCRIPT_BUILD_PLATFORM = "x64";
    static constexpr const char* SCRIPT_BUILD_SETTINGS_FILE = "script_build_settings.ini";
#pragma endregion

    ScriptLibraryLoader m_scriptLibraryLoader;

    std::future<bool> m_scriptBuildFuture;
    ScriptReloadState m_scriptReloadState = ScriptReloadState::Idle;

    ScriptBuildSettings m_buildSettings;

};
