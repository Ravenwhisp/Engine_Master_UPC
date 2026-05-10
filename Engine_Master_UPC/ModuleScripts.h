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

    bool buildAndReloadGameScriptsDll();

    void instantiateSceneScripts();
    void destroySceneScripts();

    bool loadScriptBuildSettings();
    bool saveScriptBuildSettings() const;

    ScriptBuildSettings& getScriptBuildSettings() { return m_buildSettings; }
    const ScriptBuildSettings& getScriptBuildSettings() const { return m_buildSettings; }

    bool isGameScriptsLoaded() const { return m_gameScriptsModule != nullptr; }

private: 
    struct ScriptReloadInfo
    {
        ScriptComponent* component = nullptr;
        std::string scriptName;
        rapidjson::Document fields;
    };

private:
    bool loadGameScriptsDll();
    bool unloadGameScriptsDll();
    bool buildGameScriptsProject();

    bool isRuntimeScriptFile(const std::filesystem::directory_entry& entry) const;
    void cleanRuntimeScriptFiles();

    unsigned int getNextReloadVersion();
    std::string buildRuntimeDllPath(unsigned int version) const;
    std::string buildRuntimePdbPath(unsigned int version) const;
    bool copyFileToRuntimePath(const std::string& sourcePath, const std::string& runtimePath);

    std::vector<ScriptReloadInfo> saveSceneScriptReloadInfo();
    void restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos);

private:
    HMODULE m_gameScriptsModule = nullptr;

    static constexpr const char* SCRIPT_DLL_NAME = "GameScripts.dll";
    static constexpr const char* RUNTIME_DLL_PREFIX = "GameScripts_runtime_";
    static constexpr const char* SCRIPT_PDB_NAME = "GameScripts.pdb";
    static constexpr const char* SCRIPT_BUILD_CONFIGURATION = "Debug";
    static constexpr const char* SCRIPT_BUILD_PLATFORM = "x64";
    static constexpr const char* SCRIPT_BUILD_SETTINGS_FILE = "script_build_settings.ini";

    ScriptBuildSettings m_buildSettings;

    std::string m_loadedDllPath;
    unsigned int m_reloadVersion = 0;
};
