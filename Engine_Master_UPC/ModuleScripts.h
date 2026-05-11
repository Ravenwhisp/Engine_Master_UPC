#pragma once

#include "Module.h"

#include <windef.h>
#include <string>
#include <rapidjson/document.h>
#include <vector>
#include <filesystem>

class ScriptComponent;

struct ScriptBuildSettings
{
    std::string projectPath;
    std::string solutionDir;
};

class ModuleScripts : public Module
{
public:
    ModuleScripts();
    ~ModuleScripts() override = default;

    bool init() override;
    bool cleanUp() override;

#pragma region ScriptRuntime
    bool buildAndReloadGameScriptsDll();

    void instantiateSceneScripts();
    void destroySceneScripts();

    bool isGameScriptsLoaded() const { return m_gameScriptsModule != nullptr; }
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
    bool loadGameScriptsDll();
    bool unloadGameScriptsDll();

    unsigned int getNextReloadVersion();
    std::string buildRuntimeDllPath(unsigned int version) const;
    std::string buildRuntimePdbPath(unsigned int version) const;
    bool copyFileToRuntimePath(const std::string& sourcePath, const std::string& runtimePath);
#pragma endregion

#pragma region ScriptBuild
    bool buildGameScriptsProject();
    std::filesystem::path resolveBuildPath(const std::string& path) const;

    bool validateScriptBuildPaths(const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir) const;
    bool writeScriptsBuildBatchFile(const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir, const std::filesystem::path& msbuildPath, const std::string& buildBatPath) const;
#pragma endregion

#pragma region RuntimeCleanup
    bool isRuntimeScriptFile(const std::filesystem::directory_entry& entry) const;
    void cleanRuntimeScriptFiles();
#pragma endregion

#pragma region ScriptReloadState
    std::vector<ScriptReloadInfo> saveSceneScriptReloadInfo();
    void restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos);
#pragma endregion

private:
#pragma region Constants
    static constexpr const char* SCRIPT_DLL_NAME = "GameScripts.dll";
    static constexpr const char* RUNTIME_DLL_PREFIX = "GameScripts_runtime_";
    static constexpr const char* SCRIPT_PDB_NAME = "GameScripts.pdb";
    static constexpr const char* SCRIPT_BUILD_CONFIGURATION = "Debug";
    static constexpr const char* SCRIPT_BUILD_PLATFORM = "x64";
    static constexpr const char* SCRIPT_BUILD_SETTINGS_FILE = "script_build_settings.ini";
#pragma endregion

    HMODULE m_gameScriptsModule = nullptr;
    std::string m_loadedDllPath;
    unsigned int m_reloadVersion = 0;

    ScriptBuildSettings m_buildSettings;

};
