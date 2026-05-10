#pragma once

#include "Module.h"

#include <windef.h>
#include <string>
#include <rapidjson/document.h>
#include <vector>

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

    unsigned int getNextReloadVersion();
    std::string buildRuntimeDllPath(unsigned int version) const;
    std::string buildRuntimePdbPath(unsigned int version) const;
    bool copyFileToRuntimePath(const std::string& sourcePath, const std::string& runtimePath);
    bool patchRuntimeDllPdbPath(const std::string& runtimeDllPath, const std::string& runtimePdbPath);

    std::vector<ScriptReloadInfo> saveSceneScriptReloadInfo();
    void restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos);

private:
    HMODULE m_gameScriptsModule = nullptr;

    static constexpr const char* SCRIPT_DLL_NAME = "GameScripts.dll";
    static constexpr const char* RUNTIME_DLL_PREFIX = "GameScripts_runtime_";
    static constexpr const char* SCRIPT_PDB_NAME = "GameScripts.pdb";
    static constexpr const char* SCRIPT_BUILD_CONFIGURATION = "Debug";
    static constexpr const char* SCRIPT_BUILD_PLATFORM = "x64";

    ScriptBuildSettings m_buildSettings;

    std::string m_loadedDllPath;
    unsigned int m_reloadVersion = 0;
};
