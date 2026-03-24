#include "Globals.h"
#include "Application.h"

#include "ModuleInput.h"
#include "ModuleD3D12.h"
#include "ModuleEditor.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include "ModuleDescriptors.h"
#include "ModuleUI.h"
#include "ModuleRender.h"
#include "ModuleScene.h"
#include "ModuleFileSystem.h"
#include "ModuleAssets.h"
#include "ModuleEventSystem.h"
#include "ModuleGameView.h"
#include "ModuleNavigation.h"
#include "ModuleTime.h"
#include "ScriptFactory.h"

#include "Settings.h"
#include "PerformanceProfiler.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
    : m_hWnd((HWND)hWnd)
{
    modules.push_back(m_moduleTime = new ModuleTime(120));
    modules.push_back(m_moduleInput = new ModuleInput((HWND)hWnd));
    modules.push_back(m_moduleD3d12M = new ModuleD3D12((HWND)hWnd));
    modules.push_back(m_moduleDescriptors = new ModuleDescriptors(m_moduleD3d12M->getDevice()));
    modules.push_back(m_moduleResources = new ModuleResources(m_moduleD3d12M->getDevice(), m_moduleD3d12M->getCommandQueue()));

    //Needed to create the LOGs
    modules.push_back(m_moduleEditor = new ModuleEditor());

    modules.push_back(m_moduleFileSystem = new ModuleFileSystem());
    modules.push_back(m_moduleAssets = new ModuleAssets());
    modules.push_back(m_eventSystemModule = new ModuleEventSystem());

    modules.push_back(m_moduleUI = new ModuleUI());
    modules.push_back(m_moduleNavigation = new ModuleNavigation());
    modules.push_back(m_moduleRender = new ModuleRender());
    
    modules.push_back(m_moduleGameView = new ModuleGameView());

    modules.push_back(m_moduleCamera = new ModuleCamera());
    modules.push_back(m_moduleScene = new ModuleScene());

    m_settings = new Settings();
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

    m_lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // DLL TEST
    m_gameScriptsModule = LoadLibraryA("GameScripts.dll");
    assert(m_gameScriptsModule != nullptr);

    //DELL TEST

	return ret;
}


void Application::update()
{
    auto frameStart = std::chrono::high_resolution_clock::now();

    float dt = 0.f;
    if (m_currentEngineState == ENGINE_STATE::PLAYING)
    {
        dt = m_moduleTime->deltaTime();
    }

    if (!app->m_paused)
    {
        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->update();
        }

        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->preRender();
        }

        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->render();
        }

        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->postRender();
        }
    }

    const PerfDataMap& data = getPerfData();

    /*
    for (const auto& [name, perf] : data)
    {
        DEBUG_LOG("%s -> last: %.3f ms | avg: %.3f ms | max: %.3f ms\n",
            name.c_str(),
            perf.lastMs,
            perf.avgMs,
            perf.maxMs);
    }
    */

    auto frameEnd = std::chrono::high_resolution_clock::now();

    m_elapsedMilis = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();

    m_moduleTime->waitForNextFrame();
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
