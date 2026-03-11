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
#include "PerformanceProfiler.h"
#include <thread>

#include "Settings.h"

using namespace std::chrono;


Application::Application(int argc, wchar_t** argv, void* hWnd)
    : m_hWnd((HWND)hWnd)
{
    modules.push_back(m_inputModule = new ModuleInput((HWND)hWnd));
    modules.push_back(m_d3d12Module = new ModuleD3D12((HWND)hWnd));
    modules.push_back(m_descriptorsModule = new ModuleDescriptors());
    modules.push_back(m_resourcesModule = new ModuleResources());

    //Needed to create the LOGs
    modules.push_back(m_editorModule = new ModuleEditor());

    modules.push_back(m_assetsModule = new ModuleAssets());
    modules.push_back(m_fileSystemModule = new ModuleFileSystem());
    modules.push_back(m_moduleEventSystem = new ModuleEventSystem());

    modules.push_back(m_uiModule = new ModuleUI());
    modules.push_back(m_navigationModule = new ModuleNavigation());
    modules.push_back(m_renderModule = new ModuleRender());
    
    modules.push_back(m_gameViewModule = new ModuleGameView());

    modules.push_back(m_cameraModule = new ModuleCamera());
    modules.push_back(m_sceneModule = new ModuleScene());
    modules.push_back(m_timeModule = new ModuleTime(120));

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

    PERF_BEGIN("Engine Init");
	for(auto it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->init();
    PERF_END("Engine Init");

    m_lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	return ret;
}

bool Application::postInit()
{
    bool ret = true;

    for (auto it = modules.begin(); it != modules.end() && ret; ++it)
        ret = (*it)->postInit();

    return ret;
}

void Application::update()
{
    auto frameStart = std::chrono::high_resolution_clock::now();

    float dt = 0.f;
    if (m_currentEngineState == ENGINE_STATE::PLAYING)
    {
        dt = m_timeModule->deltaTime();
    }

    if (!app->m_paused)
    {
        PERF_BEGIN("Engine Update");
        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->update();
        }
        PERF_END("Engine Update");

        PERF_BEGIN("Engine Prerender");
        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->preRender();
        }
        PERF_END("Engine Prerender");

        PERF_BEGIN("Engine Render");
        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->render();
        }
        PERF_END("Engine Render");

        PERF_BEGIN("Engine Postrender");
        for (auto it = modules.begin(); it != modules.end(); ++it)
        {
            (*it)->postRender();
        }
        PERF_END("Engine Postrender");
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

    //m_timeModule->waitForNextFrame();
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
