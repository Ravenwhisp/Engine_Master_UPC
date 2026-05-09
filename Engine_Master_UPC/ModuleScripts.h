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

    bool isGameScriptsLoaded() const { return m_gameScriptsModule != nullptr; }

private:
    HMODULE m_gameScriptsModule = nullptr;
    std::string m_loadedDllPath;
};
