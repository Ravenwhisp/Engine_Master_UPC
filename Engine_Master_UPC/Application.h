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
class UIModule;
class RenderModule;
class NavigationModule;
class SceneModule;
class GameViewModule;
class FileSystemModule;
class AssetsModule;
class ModuleEventSystem;
class ModuleFlyweight;

class CameraComponent;

class Settings;

enum ENGINE_STATE
{
    PLAYING,
    PAUSED,
    EDITOR
};

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
    UIModule*                   getUIModule() { return m_uiModule; }
    RenderModule*               getRenderModule() { return m_renderModule; }
    NavigationModule*           getNavigationModule() { return m_navigationModule; }
    SceneModule*                getSceneModule() { return m_sceneModule; }
    GameViewModule*             getGameViewModule() { return m_gameViewModule; }
    FileSystemModule*           getFileSystemModule() { return m_fileSystemModule; }
    AssetsModule*               getAssetModule() { return m_assetsModule; }
    ModuleEventSystem*          getModuleEventSystem() { return m_moduleEventSystem; }
    ModuleFlyweight*			getModuleFlyweight() { return m_moduleFlyweight; }

    Settings*                   getSettings() { return m_settings; }

    // FIXME: Cannot return const CameraComponent* (which it should) because render is not const
    const CameraComponent* getCurrentCameraPerspective() const { return m_currentCameraPerspective; }
    void setCurrentCameraPerspective(CameraComponent* camera) { m_currentCameraPerspective = camera; }

	const ENGINE_STATE getCurrentEngineState() const { return m_currentEngineState; }
	void setEngineState(int index) { m_currentEngineState = static_cast<ENGINE_STATE>(index); }

    bool        isPaused() const { return m_paused; }
    bool        setPaused(bool p) { m_paused = p; return m_paused; }

    void requestApplicationExit() { m_quit = true; }
    bool shouldQuit() const { return m_quit; }

    HWND getWindowHandle() const { return m_hWnd; }

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
    NavigationModule*       m_navigationModule = nullptr;
    SceneModule*            m_sceneModule = nullptr;
    GameViewModule*         m_gameViewModule = nullptr;
    FileSystemModule*       m_fileSystemModule = nullptr;
    AssetsModule*           m_assetsModule = nullptr;
    UIModule*               m_uiModule = nullptr;
    ModuleEventSystem*      m_moduleEventSystem = nullptr;
    ModuleFlyweight*		m_moduleFlyweight = nullptr;

    Settings*               m_settings = nullptr;

    bool m_paused = false;
    bool m_quit = false;

    ENGINE_STATE m_currentEngineState = ENGINE_STATE::EDITOR;

    uint64_t m_lastMilis = 0;
    uint64_t m_elapsedMilis = 0;

    HWND m_hWnd = nullptr;

    // This is the current camera perspective, to check a CameraComponent's perspective from the scene editor
    CameraComponent* m_currentCameraPerspective = nullptr;
};

extern Application* app;
