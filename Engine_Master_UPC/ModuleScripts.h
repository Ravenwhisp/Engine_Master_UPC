#pragma once

#include "Module.h"

#include <windef.h>
#include <string>

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
    std::string buildRuntimeDllPath();
    bool copySourceDllToRuntimeDll(const std::string& sourceDllPath, const std::string& runtimeDllPath);

private:
    HMODULE m_gameScriptsModule = nullptr;
    std::string m_loadedDllPath;

    unsigned int m_reloadVersion = 0;
};
