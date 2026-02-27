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
class ShaderDescriptorsModule;
class DescriptorsModule;
class TimeModule;
class RenderModule;
class SceneModule;
class FileSystemModule;
class AssetsModule;

class CameraComponent;

class Settings;

class Application
{
public:
	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
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
    FileSystemModule*           getFileSystemModule() { return m_fileSystemModule; }
    AssetsModule*               getAssetModule() { return m_assetsModule; }

    Settings*                   getSettings() { return m_settings; }

    // FIXME: Cannot return const CameraComponent* (which it should) because render is not const
    CameraComponent* getActiveCamera() const { return m_activeCamera; }
    void setActiveCamera(CameraComponent* camera) { m_activeCamera = camera; }

    const CameraComponent* getCurrentCameraPerspective() const { return m_currentCameraPerspective; }
    void setCurrentCameraPerspective(CameraComponent* camera) { m_currentCameraPerspective = camera; }

    bool        isPaused() const { return m_paused; }
    bool        setPaused(bool p) { m_paused = p; return m_paused; }


    uint64_t                    getElapsedMilis() const { return m_elapsedMilis; }

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
    FileSystemModule*       m_fileSystemModule = nullptr;
    AssetsModule*           m_assetsModule = nullptr;

    Settings*               m_settings = nullptr;

    bool m_paused = false;


    uint64_t m_lastMilis = 0;
    uint64_t m_elapsedMilis = 0;

    // This will be the Game mode / Simulation mode camera
    CameraComponent* m_activeCamera = nullptr;

    // This is the current camera perspective, to check a CameraComponent's perspective from the scene editor
    CameraComponent* m_currentCameraPerspective = nullptr;
};

extern Application* app;
