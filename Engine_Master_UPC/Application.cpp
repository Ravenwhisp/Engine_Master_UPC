#include "Globals.h"
#include "Application.h"
#include "InputModule.h"
#include "D3D12Module.h"
#include "EditorModule.h"
#include "ResourcesModule.h"
#include "CameraModule.h"
#include "DescriptorsModule.h"
#include "UIModule.h"
#include "RenderModule.h"
#include "SceneModule.h"
#include "FileSystemModule.h"
#include "AssetsModule.h"
#include "ModuleEventSystem.h"
#include "GameViewModule.h"
#include "NavigationModule.h"
#include "TimeModule.h"
#include "PerformanceProfiler.h"
#include "ScriptFactory.h"
#include <thread>

#include "Settings.h"

using namespace std::chrono;


Application::Application(int argc, wchar_t** argv, void* hWnd)
    : m_hWnd((HWND)hWnd)
{
    modules.push_back(m_inputModule = new InputModule((HWND)hWnd));
    modules.push_back(m_d3d12Module = new D3D12Module((HWND)hWnd));
    modules.push_back(m_descriptorsModule = new DescriptorsModule());
    modules.push_back(m_resourcesModule = new ResourcesModule());

    //Needed to create the LOGs
    modules.push_back(m_editorModule = new EditorModule());

    modules.push_back(m_assetsModule = new AssetsModule());
    modules.push_back(m_fileSystemModule = new FileSystemModule());
    modules.push_back(m_moduleEventSystem = new ModuleEventSystem());

    modules.push_back(m_uiModule = new UIModule());
    modules.push_back(m_navigationModule = new NavigationModule());
    modules.push_back(m_renderModule = new RenderModule());
    
    modules.push_back(m_gameViewModule = new GameViewModule());

    modules.push_back(m_cameraModule = new CameraModule());
    modules.push_back(m_sceneModule = new SceneModule());
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

	for(auto it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->init();

    m_lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // DLL TEST
    HMODULE gameScriptsModule = LoadLibraryA("GameScripts.dll");
    assert(gameScriptsModule != nullptr);

    using GetScriptNameFn = const char* (*)();
    using GetScriptCreatorFn = ScriptCreator(*)();

    GetScriptNameFn getScriptName =
        (GetScriptNameFn)GetProcAddress(gameScriptsModule, "GetScriptName");
    assert(getScriptName != nullptr);

    GetScriptCreatorFn getScriptCreator =
        (GetScriptCreatorFn)GetProcAddress(gameScriptsModule, "GetScriptCreator");
    assert(getScriptCreator != nullptr);

    const char* scriptName = getScriptName();
    ScriptCreator creator = getScriptCreator();

    assert(scriptName != nullptr);
    assert(creator != nullptr);

    ScriptFactory::registerScript(scriptName, creator);
    assert(ScriptFactory::isScriptRegistered("Test"));
    //DELL TEST

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

    for (const auto& [name, perf] : data)
    {
        DEBUG_LOG("%s -> last: %.3f ms | avg: %.3f ms | max: %.3f ms\n",
            name.c_str(),
            perf.lastMs,
            perf.avgMs,
            perf.maxMs);
    }

    auto frameEnd = std::chrono::high_resolution_clock::now();

    m_elapsedMilis = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();

    m_timeModule->waitForNextFrame();
}

bool Application::cleanUp()
{
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
