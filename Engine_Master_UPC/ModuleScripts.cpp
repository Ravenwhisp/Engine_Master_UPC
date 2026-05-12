#include "Globals.h"
#include "ModuleScripts.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "ScriptComponent.h"
#include "ScriptFactory.h"

#include "PdbPatcher.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>
#include <chrono>
#include <future>

namespace
{
    std::string trim(const std::string& value)
    {
        const size_t first = value.find_first_not_of(" \t\r\n");

        if (first == std::string::npos)
        {
            return "";
        }

        const size_t last = value.find_last_not_of(" \t\r\n");

        return value.substr(first, last - first + 1);
    }
}

ModuleScripts::ModuleScripts()
{
}

bool ModuleScripts::init()
{
    loadScriptBuildSettings();

    return loadGameScriptsDll();
}

void ModuleScripts::update()
{
    updateScriptReload();
}

bool ModuleScripts::cleanUp()
{
    if (m_scriptBuildFuture.valid())
    {
        m_scriptBuildFuture.wait();
    }

    const bool unloaded = unloadGameScriptsDll();

    cleanRuntimeScriptFiles();

    return unloaded;
}

bool ModuleScripts::requestBuildAndReloadGameScriptsDll()
{
    if (isScriptReloadBusy())
    {
        DEBUG_WARN("[ModuleScripts] GameScripts build/reload is already in progress.");
        return false;
    }

    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        return false;
    }

    if (m_buildSettings.projectPath.empty())
    {
        DEBUG_ERROR("[ModuleScripts] Script project path is empty.");
        return false;
    }

    if (m_buildSettings.solutionDir.empty())
    {
        DEBUG_ERROR("[ModuleScripts] Script solution directory is empty.");
        return false;
    }

    const ScriptBuildSettings buildSettings = m_buildSettings;

    // The build runs in a worker thread, but DLL reload and script restoration
    // must stay on the main thread because they touch scene/editor state.

    m_scriptReloadState = ScriptReloadState::Building;

    m_scriptBuildFuture = std::async(std::launch::async, [this, buildSettings]()
        {
            return buildGameScriptsProject(buildSettings);
        });

    return true;
}

void ModuleScripts::updateScriptReload()
{
    if (m_scriptReloadState != ScriptReloadState::Building)
    {
        return;
    }

    if (!m_scriptBuildFuture.valid())
    {
        m_scriptReloadState = ScriptReloadState::BuildFailed;
        DEBUG_ERROR("[ModuleScripts] GameScripts build failed. Current scripts remain loaded.");
        return;
    }

    const std::future_status status = m_scriptBuildFuture.wait_for(std::chrono::seconds(0));

    if (status != std::future_status::ready)
    {
        return;
    }

    const bool buildSucceeded = m_scriptBuildFuture.get();

    if (!buildSucceeded)
    {
        m_scriptReloadState = ScriptReloadState::BuildFailed;
        DEBUG_ERROR("[ModuleScripts] GameScripts build failed. Current scripts remain loaded.");
        return;
    }

    m_scriptReloadState = ScriptReloadState::Reloading;

    if (!reloadGameScriptsDllAfterSuccessfulBuild())
    {
        m_scriptReloadState = ScriptReloadState::ReloadFailed;
        DEBUG_ERROR("[ModuleScripts] GameScripts reload failed.");
        return;
    }

    m_scriptReloadState = ScriptReloadState::Completed;
    DEBUG_LOG("[ModuleScripts] GameScripts build and reload completed successfully.");
}

void ModuleScripts::instantiateSceneScripts()
{
    const std::vector<ScriptComponent*>& scriptComponents = app->getModuleScene()->getScriptComponents();

    for (ScriptComponent* scriptComponent : scriptComponents)
    {
        if (!scriptComponent || scriptComponent->getScriptName().empty())
        {
            continue;
        }

        if (!scriptComponent->getScript())
        {
            bool created = scriptComponent->createScriptInstance();

            if (!created)
            {
                DEBUG_ERROR("[ModuleScripts] Failed to create script: %s", scriptComponent->getScriptName().c_str());
                continue;
            }
        }

        scriptComponent->resetStartState();
    }
}

void ModuleScripts::destroySceneScripts()
{
    const std::vector<ScriptComponent*>& scriptComponents = app->getModuleScene()->getScriptComponents();

    for (ScriptComponent* scriptComponent : scriptComponents)
    {
        if (!scriptComponent)
        {
            continue;
        }

        scriptComponent->destroyScriptInstance();
    }
}

bool ModuleScripts::isScriptReloadBusy() const
{
    bool busy = false;

    if (m_scriptReloadState == ScriptReloadState::Building || m_scriptReloadState == ScriptReloadState::Reloading)
    {
        busy = true;
    }

    return busy;
}

void ModuleScripts::clearScriptReloadResult()
{
    if (m_scriptReloadState == ScriptReloadState::Completed || m_scriptReloadState == ScriptReloadState::BuildFailed || m_scriptReloadState == ScriptReloadState::ReloadFailed)
    {
        m_scriptReloadState = ScriptReloadState::Idle;
    }
}

bool ModuleScripts::loadScriptBuildSettings()
{
    std::ifstream file(SCRIPT_BUILD_SETTINGS_FILE);

    if (!file.is_open())
    {
        return false;
    }

    bool insideScriptBuildSection = false;

    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty())
        {
            continue;
        }

        if (line[0] == ';' || line[0] == '#')
        {
            continue;
        }

        if (line.front() == '[' && line.back() == ']')
        {
            const std::string sectionName = trim(line.substr(1, line.size() - 2));
            insideScriptBuildSection = sectionName == "ScriptBuild";
            continue;
        }

        if (!insideScriptBuildSection)
        {
            continue;
        }

        const size_t equalsPosition = line.find('=');

        if (equalsPosition == std::string::npos)
        {
            continue;
        }

        const std::string key = trim(line.substr(0, equalsPosition));
        const std::string value = trim(line.substr(equalsPosition + 1));

        if (key == "ProjectPath")
        {
            m_buildSettings.projectPath = value;
        }
        else if (key == "SolutionDir")
        {
            m_buildSettings.solutionDir = value;
        }
    }

    if (m_buildSettings.projectPath.empty())
    {
        DEBUG_WARN("[ModuleScripts] Script build ProjectPath is empty.");
        return false;
    }

    if (m_buildSettings.solutionDir.empty())
    {
        DEBUG_WARN("[ModuleScripts] Script build SolutionDir is empty.");
        return false;
    }

    return true;
}

bool ModuleScripts::saveScriptBuildSettings() const
{
    std::ofstream file(SCRIPT_BUILD_SETTINGS_FILE, std::ios::trunc);

    if (!file.is_open())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to create script build settings file: %s", SCRIPT_BUILD_SETTINGS_FILE);
        return false;
    }

    file << "[ScriptBuild]\n";
    file << "ProjectPath=" << m_buildSettings.projectPath << "\n";
    file << "SolutionDir=" << m_buildSettings.solutionDir << "\n";

    file.close();

    DEBUG_LOG("[ModuleScripts] Script build settings saved to %s", SCRIPT_BUILD_SETTINGS_FILE);

    return true;
}

bool ModuleScripts::loadGameScriptsDll()
{
    if (m_gameScriptsModule != nullptr)
    {
        DEBUG_WARN("[ModuleScripts] GameScripts DLL is already loaded.");
        return true;
    }

    const unsigned int reloadVersion = getNextReloadVersion();

    const std::string sourceDllPath = SCRIPT_DLL_NAME;
    const std::string sourcePdbPath = SCRIPT_PDB_NAME;

    const std::string runtimeDllPath = buildRuntimeDllPath(reloadVersion);
    const std::string runtimePdbPath = buildRuntimePdbPath(reloadVersion);

    if (!copyFileToRuntimePath(sourceDllPath, runtimeDllPath))
    {
        return false;
    }

    if (std::filesystem::exists(sourcePdbPath))
    {
        if (!copyFileToRuntimePath(sourcePdbPath, runtimePdbPath))
        {
            DEBUG_ERROR("[ModuleScripts] Failed to copy PDB to runtime path.");
            return false;
        }

        if (!PdbPatcher::patchRuntimeDllPdbPath(runtimeDllPath, runtimePdbPath))
        {
            DEBUG_ERROR("[ModuleScripts] Failed to patch runtime DLL PDB path.");
            return false;
        }
    }
    else
    {
        DEBUG_WARN("[ModuleScripts] Source PDB not found: %s", sourcePdbPath.c_str());
    }

    m_gameScriptsModule = LoadLibraryA(runtimeDllPath.c_str());

    if (m_gameScriptsModule == nullptr)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to load %s", runtimeDllPath.c_str());
        return false;
    }

    m_loadedDllPath = runtimeDllPath;

    return true;
}

bool ModuleScripts::unloadGameScriptsDll()
{
    if (m_gameScriptsModule == nullptr)
    {
        return true;
    }

    ScriptFactory::clear();

    if (!FreeLibrary(m_gameScriptsModule))
    {
        DEBUG_ERROR("[ModuleScripts] Failed to unload %s", m_loadedDllPath.c_str());
        return false;
    }

    m_gameScriptsModule = nullptr;
    m_loadedDllPath.clear();

    return true;
}

bool ModuleScripts::reloadGameScriptsDllAfterSuccessfulBuild()
{
    std::vector<ScriptReloadInfo> reloadInfos = saveSceneScriptReloadInfo();

    destroySceneScripts();

    if (!unloadGameScriptsDll())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to unload current GameScripts DLL.");
        return false;
    }

    if (!loadGameScriptsDll())
    {
        DEBUG_ERROR("[ModuleScripts] Failed to load new GameScripts DLL after build.");
        return false;
    }

    instantiateSceneScripts();
    restoreSceneScriptReloadInfo(reloadInfos);
    app->getModuleScene()->getScene()->fixSceneReferences();

    return true;
}

bool ModuleScripts::buildGameScriptsProject(const ScriptBuildSettings& buildSettings) const
{
    if (buildSettings.projectPath.empty())
    {
        DEBUG_ERROR("[ModuleScripts] Script project path is empty.");
        return false;
    }

    if (buildSettings.solutionDir.empty())
    {
        DEBUG_ERROR("[ModuleScripts] Script solution directory is empty.");
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
        DEBUG_ERROR("[ModuleScripts] MSBuild.exe not found: %s", msbuildPath.string().c_str());
        return false;
    }

    const std::string buildLogPath = "ScriptsBuild.log";

    if (!runMsBuild(msbuildPath, projectPath, solutionDir, buildLogPath))
    {
        DEBUG_ERROR("[ModuleScripts] Build output written to %s", buildLogPath.c_str());
        return false;
    }

    return true;
}

std::filesystem::path ModuleScripts::resolveBuildPath(const std::string& path) const
{
    std::filesystem::path resolvedPath(path);

    if (resolvedPath.is_relative())
    {
        resolvedPath = std::filesystem::current_path() / resolvedPath;
    }

    return resolvedPath.lexically_normal();
}

bool ModuleScripts::validateScriptBuildPaths(const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir) const
{
    if (!std::filesystem::exists(projectPath) || !std::filesystem::is_regular_file(projectPath))
    {
        DEBUG_ERROR("[ModuleScripts] Script project path is invalid: %s", projectPath.string().c_str());
        return false;
    }

    if (projectPath.extension() != ".vcxproj")
    {
        DEBUG_ERROR("[ModuleScripts] Script project path must point to a .vcxproj file: %s", projectPath.string().c_str());
        return false;
    }

    if (!std::filesystem::exists(solutionDir) || !std::filesystem::is_directory(solutionDir))
    {
        DEBUG_ERROR("[ModuleScripts] Script solution directory is invalid: %s", solutionDir.string().c_str());
        return false;
    }

    return true;
}

bool ModuleScripts::runMsBuild(const std::filesystem::path& msbuildPath, const std::filesystem::path& projectPath, const std::filesystem::path& solutionDir, const std::string& buildLogPath) const
{
    const std::filesystem::path absoluteLogPath = std::filesystem::absolute(buildLogPath).lexically_normal();

    std::string solutionDirString = solutionDir.string();

    if (!solutionDirString.empty() && solutionDirString.back() == '\\')
    {
        solutionDirString += '\\';
    }

    std::string commandLine = "\"" + msbuildPath.string() + "\" " + "\"" + projectPath.string() + "\" " + "\"/p:Configuration=" + SCRIPT_BUILD_CONFIGURATION + "\" " + "\"/p:Platform=" + SCRIPT_BUILD_PLATFORM + "\" " + "\"/p:SolutionDir=" + solutionDirString + "\"";

    SECURITY_ATTRIBUTES securityAttributes = {};
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = TRUE;
    securityAttributes.lpSecurityDescriptor = nullptr;

    HANDLE logFileHandle = CreateFileA(absoluteLogPath.string().c_str(), GENERIC_WRITE, FILE_SHARE_READ, &securityAttributes, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (logFileHandle == INVALID_HANDLE_VALUE)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to create build log file. Win32 error: %lu", GetLastError());
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
        DEBUG_ERROR("[ModuleScripts] Failed to start MSBuild process. Win32 error: %lu", GetLastError());
        return false;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode = 0;
    const BOOL gotExitCode = GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    if (!gotExitCode)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to get MSBuild exit code. Win32 error: %lu", GetLastError());
        return false;
    }

    if (exitCode != 0)
    {
        return false;
    }

    return true;
}

unsigned int ModuleScripts::getNextReloadVersion()
{
    ++m_reloadVersion;
    return m_reloadVersion;
}

std::string ModuleScripts::buildRuntimeDllPath(unsigned int version) const
{
    return std::string(RUNTIME_DLL_PREFIX) + std::to_string(version) + ".dll";
}

std::string ModuleScripts::buildRuntimePdbPath(unsigned int version) const
{
    char buffer[16];
    sprintf_s(buffer, "GS_%08X.pdb", version);
    return buffer;
}

bool ModuleScripts::copyFileToRuntimePath(const std::string& sourcePath, const std::string& runtimePath)
{
    try
    {
        std::filesystem::copy_file(sourcePath, runtimePath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        DEBUG_ERROR("[ModuleScripts] Failed to copy %s to %s. Error: %s", sourcePath.c_str(), runtimePath.c_str(), e.what());

        return false;
    }

    return true;
}

bool ModuleScripts::isRuntimeScriptFile(const std::filesystem::directory_entry& entry) const
{
    if (!entry.is_regular_file())
    {
        return false;
    }

    const std::string fileName = entry.path().filename().string();

    const bool isRuntimeDll = fileName.rfind(RUNTIME_DLL_PREFIX, 0) == 0 && entry.path().extension() == ".dll";

    const bool isRuntimePdb = fileName.rfind("GS_", 0) == 0 && entry.path().extension() == ".pdb";

    return isRuntimeDll || isRuntimePdb;
}

void ModuleScripts::cleanRuntimeScriptFiles()
{
    const std::filesystem::path outputDirectory = ".";

    try
    {
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(outputDirectory))
        {
            if (!isRuntimeScriptFile(entry))
            {
                continue;
            }

            std::filesystem::remove(entry.path());
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        DEBUG_WARN("[ModuleScripts] Failed to clean runtime script files. Error: %s", e.what());
    }
}

std::vector<ModuleScripts::ScriptReloadInfo> ModuleScripts::saveSceneScriptReloadInfo()
{
    const std::vector<ScriptComponent*>& scriptComponents = app->getModuleScene()->getScriptComponents();

    std::vector<ScriptReloadInfo> reloadInfo;
    reloadInfo.reserve(scriptComponents.size());

    for (ScriptComponent* scriptComponent : scriptComponents)
    {
        if (!scriptComponent || scriptComponent->getScriptName().empty())
        {
            continue;
        }

        ScriptReloadInfo info;
        info.component = scriptComponent;
        info.scriptName = scriptComponent->getScriptName();
        info.fields.SetObject();

        rapidjson::Value fieldsJson = scriptComponent->serializeScriptFieldsForReload(info.fields);
        info.fields.Swap(fieldsJson);

        reloadInfo.push_back(std::move(info));
    }

    return reloadInfo;
}

void ModuleScripts::restoreSceneScriptReloadInfo(std::vector<ScriptReloadInfo>& reloadInfos)
{
    for (ScriptReloadInfo& info : reloadInfos)
    {
        if (!info.component)
        {
            continue;
        }

        info.component->setScriptName(info.scriptName);

        if (!info.component->getScript())
        {
            DEBUG_ERROR("[ModuleScripts] Cannot restore fields because script was not recreated: %s", info.scriptName.c_str());
            continue;
        }

        info.component->deserializeScriptFieldsForReload(info.fields);
    }
}
