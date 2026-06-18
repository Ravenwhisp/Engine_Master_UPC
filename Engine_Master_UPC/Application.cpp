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
#include "ModuleEventSystem.h"
#include "ModuleGameView.h"
#include "ModuleNavigation.h"
#include "ModuleTime.h"
#include "ModuleHaptics.h"
#include "ModuleMusic.h"
#include "ModuleScripting.h"

#include "ScriptFactory.h"

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
    modules.push_back(m_moduleEditor = new ModuleEditor());
    modules.push_back(m_moduleHaptics = new ModuleHaptics());

    modules.push_back(m_moduleScripting = new ModuleScripting());

    modules.push_back(m_moduleAssets = new ModuleAssets());
    modules.push_back(m_eventSystemModule = new ModuleEventSystem());

    modules.push_back(m_moduleUI = new ModuleUI());
    modules.push_back(m_moduleParticleSystem = new ModuleParticleSystem());
    modules.push_back(m_moduleNavigation = new ModuleNavigation());
    modules.push_back(m_moduleRender = new ModuleRender());
    
    modules.push_back(m_moduleGameView = new ModuleGameView());

    modules.push_back(m_moduleCamera = new ModuleCamera());
    modules.push_back(m_moduleScene = new ModuleScene());

    modules.push_back(m_moduleMusic = new ModuleMusic());

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

    std::vector<uint8_t> configData = FileIO::read("../Engine_OUT/build.cfg");
    if (configData.empty())
    {
        configData = FileIO::read("build.cfg");
    }

    if (!configData.empty())
    {
        std::string hash(reinterpret_cast<const char*>(configData.data()), configData.size());
        hash.erase(hash.find_last_not_of(" \n\r\t") + 1);

        if (!hash.empty())
        {
            UID uid = hashToUID(hash);
            if (isValidUID(uid))
            {
                AssetReference ref(uid, hash, AssetType::SCENE);
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
        {
            PERF_LOGIC("Application::ModulesUpdate");
            for (auto it = modules.begin(); it != modules.end(); ++it)
            {
                (*it)->update();
            }
        }

        {
            PERF_RENDER("Application::ModulesPreRender");
            for (auto it = modules.begin(); it != modules.end(); ++it)
            {
                (*it)->preRender();
            }
        }

        {
            PERF_RENDER("Application::ModulesRender");
            for (auto it = modules.begin(); it != modules.end(); ++it)
            {
                (*it)->render();
            }
        }

        {
            PERF_RENDER("Application::ModulesPostRender");
            for (auto it = modules.begin(); it != modules.end(); ++it)
            {
                (*it)->postRender();
            }
        }
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
