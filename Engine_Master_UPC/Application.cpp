#include "Globals.h"
#include "Application.h"
#include "InputModule.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "ResourcesModule.h"
#include "CameraModule.h"
#include "DescriptorsModule.h"
#include "RenderModule.h"
#include "SceneModule.h"
#include "GameViewModule.h"
#include "TimeModule.h"
#include "PerformanceProfiler.h"
#include <thread>

#include "Settings.h"

using namespace std::chrono;

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    modules.push_back(m_inputModule = new InputModule((HWND)hWnd));
    modules.push_back(m_d3d12Module = new D3D12Module((HWND)hWnd));
    modules.push_back(m_descriptorsModule = new DescriptorsModule());
    modules.push_back(m_resourcesModule = new ResourcesModule());

    modules.push_back(m_cameraModule = new CameraModule());
    modules.push_back(m_editorModule = new EditorModule());
    modules.push_back(m_sceneModule = new SceneModule());
    modules.push_back(m_renderModule = new RenderModule());
    modules.push_back(m_gameViewModule = new GameViewModule());

    modules.push_back(m_timeModule = new TimeModule(120));

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
    uint64_t currentMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    m_elapsedMilis = currentMilis - m_lastMilis;
    m_lastMilis = currentMilis;

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

        PERF_BEGIN("Engine Render");
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
        PERF_END("Engine Render");
    }

    m_timeModule->waitForNextFrame();
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
