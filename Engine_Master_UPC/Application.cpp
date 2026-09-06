#include "Globals.h"
#include "Application.h"

#include "ModuleInput.h"
#include "ModuleD3D12.h"
#include "ModuleEditor.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include "ModuleDescriptors.h"
#include "ModuleUI.h"
#include "ModuleParticleSystem.h"
#include "ModuleRender.h"
#include "ModuleScene.h"
#include "ModuleAssets.h"
#include "AssetReference.h"
#include "MD5.h"
#include "FileIO.h"
#include <sstream>
#include <unordered_map>
#include "ModuleEventSystem.h"
#include "ModuleGameView.h"
#include "ModuleNavigation.h"
#include "ModuleTime.h"
#include "ModuleHaptics.h"
#include "ModuleMusic.h"
#include "ModuleScripting.h"
#include "ModuleVideo.h"
#include "ModuleFont.h"

#include "GenericTypeFactory.h"

#include "Settings.h"
#include "ThreadPool.h"

#include "OptickProfiler.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
    : m_hWnd((HWND)hWnd)
{
    srand(time(0)); // To generate random numbers

    m_settings = new Settings();
    m_threadPool = new ThreadPool();

    modules.push_back(m_moduleTime = new ModuleTime(120));
    modules.push_back(m_moduleInput = new ModuleInput((HWND)hWnd));
    modules.push_back(m_moduleD3d12M = new ModuleD3D12((HWND)hWnd));
    modules.push_back(m_moduleDescriptors = new ModuleDescriptors(m_moduleD3d12M->getDevice()));
    modules.push_back(m_moduleResources = new ModuleResources(m_moduleD3d12M->getDevice(), m_moduleD3d12M->getCommandQueue()));

    //Needed to create the LOGs
    modules.push_back(m_moduleHaptics = new ModuleHaptics());

    modules.push_back(m_moduleScripting = new ModuleScripting());

    modules.push_back(m_moduleAssets = new ModuleAssets());

    modules.push_back(m_moduleEditor = new ModuleEditor());
    modules.push_back(m_eventSystemModule = new ModuleEventSystem());

    modules.push_back(m_moduleUI = new ModuleUI());
    modules.push_back(m_moduleParticleSystem = new ModuleParticleSystem());
    modules.push_back(m_moduleNavigation = new ModuleNavigation());
    modules.push_back(m_moduleRender = new ModuleRender());
    
    modules.push_back(m_moduleGameView = new ModuleGameView());

    modules.push_back(m_moduleCamera = new ModuleCamera());
    modules.push_back(m_moduleScene = new ModuleScene());

    modules.push_back(m_moduleMusic = new ModuleMusic());
    modules.push_back(m_moduleVideo = new ModuleVideo());
    modules.push_back(m_moduleFont = new ModuleFont());

}

Application::~Application()
{
    cleanUp();

	for(auto it = modules.rbegin(); it != modules.rend(); ++it)
    {
        delete *it;
    }
}
 
bool Application::init()
{
	bool ret = true;

    for (auto it = modules.begin(); it != modules.end() && ret; ++it)
    {
        ret = (*it)->init();
    }

#ifdef GAME_RELEASE
    app->setEngineState(ENGINE_STATE::PLAYING);
    std::vector<uint8_t> configData = FileIO::read(std::filesystem::current_path() / "build.cfg");

    if (!configData.empty())
    {
        std::string configStr(reinterpret_cast<const char*>(configData.data()), configData.size());
        std::istringstream stream(configStr);
        std::string line;

        if (std::getline(stream, line))
        {
            line.erase(line.find_last_not_of(" \n\r\t") + 1);
            if (!line.empty())
            {
                std::string sceneHash = line;
                UID uid = hashToUID(sceneHash);
                if (isValidUID(uid))
                {
                    if (std::getline(stream, line))
                    {
                        line.erase(line.find_last_not_of(" \n\r\t") + 1);
                        if (!line.empty())
                        {
                            AssetId initRef(GenerateUID(), line, AssetType::SOUND_BANK);
                            m_moduleMusic->loadBank(initRef);
                        }
                    }

                    std::unordered_map<std::string, std::string> sceneLibIds;
                    while (std::getline(stream, line))
                    {
                        line.erase(0, line.find_first_not_of(" \n\r\t"));
                        line.erase(line.find_last_not_of(" \n\r\t") + 1);

                        if (line.rfind("scene ", 0) != 0)
                        {
                            continue;
                        }

                        const size_t sep = line.find_last_of(' ');
                        if (sep <= 6 || sep + 1 >= line.size())
                        {
                            continue;
                        }

                        std::string name = line.substr(6, sep - 6);
                        std::string libId = line.substr(sep + 1);
                        sceneLibIds[std::move(name)] = std::move(libId);
                    }

                    m_moduleScene->setBuildSceneLibIds(std::move(sceneLibIds));

                    AssetId ref(uid, sceneHash, AssetType::SCENE);
                    m_moduleScene->loadScene(ref);
                }
                else
                {
                    DEBUG_ERROR("[Application] Invalid hash in build.cfg.");
                }
            }
            else
            {
                DEBUG_ERROR("[Application] build.cfg is empty.");
            }
        }
        else
        {
            DEBUG_ERROR("[Application] build.cfg is empty.");
        }
    }
    else
    {
        DEBUG_ERROR("[Application] build.cfg not found in ../Engine_OUT/ or current directory.");
    }
#endif

    m_lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	return ret;
}


void Application::update()
{
    PERF_FRAME("MainThread");
    PERF_LOGIC("Application::update");

    auto frameStart = std::chrono::high_resolution_clock::now();

    float dt = 0.f;
    if (m_currentEngineState == ENGINE_STATE::PLAYING)
    {
        dt = m_moduleTime->deltaTime();
    }

    if (!app->m_paused)
    {
		FrameCpuTimings frameCpuTimings{};

        {
            PERF_LOGIC("Application::ModulesUpdate");
			const auto phaseStart = std::chrono::high_resolution_clock::now();
            const bool profileModuleUpdates = m_settings->debugGame.showUpdateTimings;
            m_moduleScene->beginDetailedProfilingFrame(profileModuleUpdates);
            if (profileModuleUpdates)
            {
                m_moduleUpdateTimings.clear();
                m_moduleUpdateTimings.reserve(modules.size());
                for (Module* module : modules)
                {
                    const auto moduleStart = std::chrono::high_resolution_clock::now();
                    module->update();
                    const float cpuMs = std::chrono::duration<float, std::milli>(
                        std::chrono::high_resolution_clock::now() - moduleStart).count();

                    const char* moduleName = "Unknown module";
                    if (module == m_moduleTime) moduleName = "Time";
                    else if (module == m_moduleInput) moduleName = "Input";
                    else if (module == m_moduleD3d12M) moduleName = "D3D12";
                    else if (module == m_moduleDescriptors) moduleName = "Descriptors";
                    else if (module == m_moduleResources) moduleName = "Resources";
                    else if (module == m_moduleHaptics) moduleName = "Haptics";
                    else if (module == m_moduleScripting) moduleName = "Scripting module";
                    else if (module == m_moduleAssets) moduleName = "Assets";
                    else if (module == m_moduleEditor) moduleName = "Editor";
                    else if (module == m_eventSystemModule) moduleName = "Event system";
                    else if (module == m_moduleUI) moduleName = "UI";
                    else if (module == m_moduleParticleSystem) moduleName = "Particle systems";
                    else if (module == m_moduleNavigation) moduleName = "Navigation";
                    else if (module == m_moduleRender) moduleName = "Render module";
                    else if (module == m_moduleGameView) moduleName = "Game view";
                    else if (module == m_moduleCamera) moduleName = "Camera";
                    else if (module == m_moduleScene) moduleName = "Scene (objects + quadtrees)";
                    else if (module == m_moduleMusic) moduleName = "Music";
                    else if (module == m_moduleVideo) moduleName = "Video";
                    else if (module == m_moduleFont) moduleName = "Font";

                    m_moduleUpdateTimings.push_back({ moduleName, cpuMs });
                }
            }
			else
			{
				for (Module* module : modules)
				{
					module->update();
				}
			}
			frameCpuTimings.updateMs = std::chrono::duration<float, std::milli>(
				std::chrono::high_resolution_clock::now() - phaseStart).count();
        }

        {
            PERF_RENDER("Application::ModulesPreRender");
			const auto phaseStart = std::chrono::high_resolution_clock::now();
            for (auto it = modules.begin(); it != modules.end(); ++it)
            {
                (*it)->preRender();
            }
			frameCpuTimings.preRenderMs = std::chrono::duration<float, std::milli>(
				std::chrono::high_resolution_clock::now() - phaseStart).count();
        }

        {
            PERF_RENDER("Application::ModulesRender");
			const auto phaseStart = std::chrono::high_resolution_clock::now();
            for (auto it = modules.begin(); it != modules.end(); ++it)
            {
                (*it)->render();
            }
			frameCpuTimings.renderMs = std::chrono::duration<float, std::milli>(
				std::chrono::high_resolution_clock::now() - phaseStart).count();
        }

        {
            PERF_RENDER("Application::ModulesPostRender");
			const auto phaseStart = std::chrono::high_resolution_clock::now();
            for (auto it = modules.begin(); it != modules.end(); ++it)
            {
                (*it)->postRender();
            }
			frameCpuTimings.postRenderMs = std::chrono::duration<float, std::milli>(
				std::chrono::high_resolution_clock::now() - phaseStart).count();
        }

		m_frameCpuTimings = frameCpuTimings;
    }

    auto frameEnd = std::chrono::high_resolution_clock::now();

    m_elapsedMilis = static_cast<uint64_t>(std::chrono::duration<float, std::milli>(frameEnd - frameStart).count());

    //m_moduleTime->waitForNextFrame();
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
