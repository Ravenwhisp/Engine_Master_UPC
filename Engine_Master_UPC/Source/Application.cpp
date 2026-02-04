#include "Globals.h"
#include "Application.h"
#include "InputModule.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "ResourcesModule.h"
#include "CameraModule.h"
#include "DescriptorsModule.h"
#include "RenderModule.h"
#include "GameCoreModule.h"
#include "TimeModule.h"
#include "PerformanceProfiler.h"
#include <thread>

using namespace std::chrono;

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    modules.push_back(_inputModule = new InputModule((HWND)hWnd));
    modules.push_back(_editorModule = new EditorModule());
    modules.push_back(_d3d12 = new D3D12Module((HWND)hWnd));
    modules.push_back(_renderModule = new RenderModule());
    modules.push_back(_descriptorsModule = new DescriptorsModule());
    modules.push_back(_resourcesModule = new ResourcesModule());
    modules.push_back(_cameraModule = new CameraModule());
    modules.push_back(_timeModule = new TimeModule(120));
    modules.push_back(_gameCoreModule = new GameCoreModule());
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

    if (!app->paused)
    {
        PERF_BEGIN("Engine Update");
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->update();

        PERF_END("Engine Update");

        PERF_BEGIN("Engine Render");
        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->preRender();


        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->render();

        for (auto it = modules.begin(); it != modules.end(); ++it)
            (*it)->postRender();
        PERF_END("Engine Render");
    }

    _timeModule->WaitForNextFrame();
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
