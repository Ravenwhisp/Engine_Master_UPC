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
class SceneModule;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
    bool         postInit();
	void         update();
	bool         cleanUp();

    D3D12Module*                getD3D12Module() { return m_d3d12Module; }
    EditorModule*               getEditorModule() { return m_editorModule; }
    ResourcesModule*            getResourcesModule() { return m_resourcesModule; }
    InputModule*                getInputModule() { return m_inputModule; }
    CameraModule*               getCameraModule() { return m_cameraModule; }
    DescriptorsModule*          getDescriptorsModule() { return m_descriptorsModule; }
    TimeModule*                 getTimeModule() { return m_timeModule; }
    RenderModule*               getRenderModule() { return m_renderModule; }
    SceneModule*                getSceneModule() { return m_sceneModule; }

    bool        isPaused() const { return m_paused; }
    bool        setPaused(bool p) { m_paused = p; return m_paused; }
private:

    std::vector<Module*>    modules;
    D3D12Module*            m_d3d12Module = nullptr;
    EditorModule*           m_editorModule = nullptr;
    ResourcesModule*        m_resourcesModule = nullptr;
    CameraModule*           m_cameraModule = nullptr;
    InputModule*            m_inputModule = nullptr;
    DescriptorsModule*      m_descriptorsModule = nullptr;
    TimeModule*             m_timeModule = nullptr;
    RenderModule*           m_renderModule = nullptr;
    SceneModule*            m_sceneModule = nullptr;

    bool m_paused = false;

};

extern Application* app;
