#pragma once

#include <Windows.h>

#include <string>
#include <filesystem>

class ScriptLibraryLoader
{
public:
    bool load();
    bool unload();

    void cleanRuntimeFiles();

    bool isLoaded() const { return m_module != nullptr; }

    const std::string& getLoadedDllPath() const { return m_loadedDllPath; }

private:
    unsigned int getNextReloadVersion();

    std::string buildRuntimeDllPath(unsigned int version) const;
    std::string buildRuntimePdbPath(unsigned int version) const;

    bool copyFileToRuntimePath(const std::string& sourcePath, const std::string& runtimePath) const;

    bool isRuntimeFile(const std::filesystem::directory_entry& entry) const;

private:
    static constexpr const char* SCRIPT_DLL_NAME = "GameScripts.dll";
    static constexpr const char* RUNTIME_DLL_PREFIX = "GameScripts_runtime_";
    static constexpr const char* SCRIPT_PDB_NAME = "GameScripts.pdb";

private:
    HMODULE m_module = nullptr;
    std::string m_loadedDllPath;
    unsigned int m_reloadVersion = 0;
};