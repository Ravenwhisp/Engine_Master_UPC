#include "Globals.h"
#include "ScriptBuildSystem.h"

#include <Windows.h>

#include <filesystem>
#include <vector>

bool ScriptBuildSystem::build(const ScriptBuildSettings& buildSettings) const
{
    if (buildSettings.projectPath.empty())
    {
        DEBUG_ERROR("[ScriptBuildSystem] Script project path is empty.");
        return false;
    }

    if (buildSettings.solutionDir.empty())
    {
        DEBUG_ERROR("[ScriptBuildSystem] Script solution directory is empty.");
        return false;
    }

    if (buildSettings.configuration.empty())
    {
        DEBUG_ERROR("[ScriptBuildSystem] Script build configuration is empty.");
        return false;
    }

    if (buildSettings.platform.empty())
    {
        DEBUG_ERROR("[ScriptBuildSystem] Script build platform is empty.");
        return false;
    }

    const std::filesystem::path projectPath = resolveBuildPath(buildSettings.projectPath);
    const std::filesystem::path solutionDir = resolveBuildPath(buildSettings.solutionDir);

    if (!validateScriptBuildPaths(projectPath, solutionDir))
    {
        return false;
    }

    const std::filesystem::path msbuildPath = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe";

    if (!std::filesystem::exists(msbuildPath))
    {
        DEBUG_ERROR("[ScriptBuildSystem] MSBuild.exe not found: %s", msbuildPath.string().c_str());
        return false;
    }

    if (!runMsBuild(msbuildPath, projectPath, solutionDir, buildSettings.configuration, buildSettings.platform, BUILD_LOG_PATH))
    {
        DEBUG_ERROR("[ScriptBuildSystem] Build output written to %s", BUILD_LOG_PATH);
        return false;
    }

    return true;
}

std::filesystem::path ScriptBuildSystem::resolveBuildPath(const std::string& path) const
{
    std::filesystem::path resolvedPath(path);

    if (resolvedPath.is_relative())
    {
        resolvedPath = std::filesystem::current_path() / resolvedPath;
    }

    return resolvedPath.lexically_normal();
}

bool ScriptBuildSystem::validateScriptBuildPaths(const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir) const
{
    if (!std::filesystem::exists(projectPath) || !std::filesystem::is_regular_file(projectPath))
    {
        DEBUG_ERROR("[ScriptBuildSystem] Script project path is invalid: %s", projectPath.string().c_str());
        return false;
    }

    if (projectPath.extension() != ".vcxproj")
    {
        DEBUG_ERROR("[ScriptBuildSystem] Script project path must point to a .vcxproj file: %s", projectPath.string().c_str());
        return false;
    }

    if (!std::filesystem::exists(solutionDir) || !std::filesystem::is_directory(solutionDir))
    {
        DEBUG_ERROR("[ScriptBuildSystem] Script solution directory is invalid: %s", solutionDir.string().c_str());
        return false;
    }

    return true;
}

// Launch MSBuild without opening a cmd window
bool ScriptBuildSystem::runMsBuild(const std::filesystem::path& msbuildPath, const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir, const std::string& configuration, const std::string& platform, const std::string& buildLogPath) const
{
    const std::filesystem::path absoluteLogPath = std::filesystem::absolute(buildLogPath).lexically_normal();

    std::string solutionDirString = solutionDir.string();

    if (!solutionDirString.empty() && solutionDirString.back() != '\\' && solutionDirString.back() != '/')
    {
        solutionDirString += '\\';
    }

    std::string quotedSolutionDirString = solutionDirString;

    if (!quotedSolutionDirString.empty() && quotedSolutionDirString.back() == '\\')
    {
        quotedSolutionDirString += '\\';
    }

    const std::string commandLine =
        "\"" + msbuildPath.string() + "\" " +
        "\"" + projectPath.string() + "\" " +
        "\"/t:Build\" " +
        "\"/p:Configuration=" + configuration + "\" " +
        "\"/p:Platform=" + platform + "\" " +
        "\"/p:SolutionDir=" + quotedSolutionDirString + "\"";

    SECURITY_ATTRIBUTES securityAttributes = {};
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = TRUE;
    securityAttributes.lpSecurityDescriptor = nullptr;

    HANDLE logFileHandle = CreateFileA(absoluteLogPath.string().c_str(), GENERIC_WRITE, FILE_SHARE_READ, &securityAttributes, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (logFileHandle == INVALID_HANDLE_VALUE)
    {
        DEBUG_ERROR("[ScriptBuildSystem] Failed to create build log file. Win32 error: %lu", GetLastError());
        return false;
    }

    STARTUPINFOA startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startupInfo.hStdOutput = logFileHandle;
    startupInfo.hStdError = logFileHandle;
    startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startupInfo.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION processInfo = {};

    std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back('\0');

    const BOOL created = CreateProcessA(nullptr, mutableCommandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);

    CloseHandle(logFileHandle);

    if (!created)
    {
        DEBUG_ERROR("[ScriptBuildSystem] Failed to start MSBuild process. Win32 error: %lu", GetLastError());
        return false;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode = 0;
    const BOOL gotExitCode = GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (!gotExitCode)
    {
        DEBUG_ERROR("[ScriptBuildSystem] Failed to get MSBuild exit code. Win32 error: %lu", GetLastError());
        return false;
    }

    return exitCode == 0;
}