#pragma once

#include "Module.h"

#include <windef.h>
#include <string>
#include <rapidjson/document.h>
#include <vector>

class ScriptComponent;

class ModuleScripts : public Module
{
public:
    ModuleScripts();
    ~ModuleScripts() override = default;

    bool init() override;
    bool cleanUp() override;

    bool loadGameScriptsDll();
    bool unloadGameScriptsDll();
    bool reloadGameScriptsDll();

    void instantiateSceneScripts();
    void destroySceneScripts();

    bool isGameScriptsLoaded() const { return m_gameScriptsModule != nullptr; }

private: 
    struct ScriptReloadInfo
    {
        ScriptComponent* component = nullptr;
        std::string scriptName;
        rapidjson::Document fields;
    };

private:
    std::string buildRuntimeDllPath();
    bool copySourceDllToRuntimeDll(const std::string& sourceDllPath, const std::string& runtimeDllPath);

    std::vector<ScriptReloadInfo> saveSceneScriptReloadInfo();
    void restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos);

private:
    HMODULE m_gameScriptsModule = nullptr;
    std::string m_loadedDllPath;

    unsigned int m_reloadVersion = 0;
};
