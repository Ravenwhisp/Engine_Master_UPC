#pragma once

#include <filesystem>
#include <string>

struct ScriptBuildSettings
{
    std::string projectPath;
    std::string solutionDir;
};

class ScriptBuildSystem
{
public:
    bool build(const ScriptBuildSettings& buildSettings) const;

private:
    std::filesystem::path resolveBuildPath(const std::string& path) const;
    bool validateScriptBuildPaths(const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir) const;

    bool runMsBuild(const std::filesystem::path& msbuildPath, const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir, const std::string& buildLogPath) const;

private:
#if defined(_DEBUG)
    static constexpr const char* SCRIPT_BUILD_CONFIGURATION = "Debug";
#else
    static constexpr const char* SCRIPT_BUILD_CONFIGURATION = "Release";
#endif

    static constexpr const char* SCRIPT_BUILD_PLATFORM = "x64";
    static constexpr const char* BUILD_LOG_PATH = "ScriptsBuild.log";
};