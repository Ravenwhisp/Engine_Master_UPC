#pragma once

#include "Globals.h"

#include <array>
#include <vector>
#include <chrono>

class Module;
class D3D12Module;
class EditorModule;
class ResourcesModule;
class CameraModule;
class InputModule;
class SampleModule;
class ShaderDescriptorsModule;
class DescriptorsModule;
class TimeModule;
class RenderModule;
class GameCoreModule;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
    bool         postInit();
	void         update();
	bool         cleanUp();

    D3D12Module*                GetD3D12Module() { return _d3d12; }
    EditorModule*              GetEditorModule() { return _editorModule; }
    ResourcesModule*            GetResourcesModule() { return _resourcesModule; }
    InputModule*               GetInputModule() { return _inputModule; }
    CameraModule* GetCameraModule() { return _cameraModule; }
    DescriptorsModule* GetDescriptorsModule() { return _descriptorsModule; }
    TimeModule*                GetTimeModule() { return _timeModule; }
    RenderModule* GetRenderModule() { return _renderModule; }
    GameCoreModule*            GetGameCoreModule() { return _gameCoreModule; }

    bool                        isPaused() const { return paused; }
    bool                        setPaused(bool p) { paused = p; return paused; }
private:

    std::vector<Module*> modules;
    D3D12Module* _d3d12 = nullptr;
    EditorModule* _editorModule = nullptr;
    ResourcesModule* _resourcesModule = nullptr;
    CameraModule* _cameraModule = nullptr;
    InputModule* _inputModule = nullptr;
    DescriptorsModule* _descriptorsModule = nullptr;
    TimeModule* _timeModule = nullptr;
    RenderModule* _renderModule = nullptr;
    GameCoreModule* _gameCoreModule = nullptr;

    bool      paused = false;

};

extern Application* app;
