#pragma once

#include <filesystem>
#include <string>

struct ScriptBuildSettings
{
    std::string projectPath;
    std::string solutionDir;

    std::string configuration = "Release";
    std::string platform = "x64";
};

class ScriptBuildSystem
{
public:
    bool build(const ScriptBuildSettings& buildSettings) const;

private:
    std::filesystem::path resolveBuildPath(const std::string& path) const;
    bool validateScriptBuildPaths(const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir) const;

    bool runMsBuild(const std::filesystem::path& msbuildPath, const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir, const std::string& configuration, const std::string& platform, const std::string& buildLogPath) const;

private:
    static constexpr const char* BUILD_LOG_PATH = "ScriptsBuild.log";

};