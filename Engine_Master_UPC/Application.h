#pragma once
#include "Globals.h"

#include <array>
#include <vector>
#include <chrono>

class Module;
class ModuleD3D12;
class ModuleEditor;
class ModuleResources;
class ModuleCamera;
class ModuleInput;
class ShaderModuleDescriptors;
class ModuleDescriptors;
class ModuleTime;
class ModuleUI;
class ModuleRender;
class ModuleNavigation;
class ModuleScene;
class ModuleGameView;
class ModuleFileSystem;
class ModuleAssets;
class ModuleEventSystem;

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

    ModuleD3D12*                getModuleD3D12() { return m_d3d12Module; }
    ModuleEditor*               getModuleEditor() { return m_editorModule; }
    ModuleResources*            getModuleResources() { return m_resourcesModule; }
    ModuleInput*                getModuleInput() { return m_inputModule; }
    ModuleCamera*               getModuleCamera() { return m_cameraModule; }
    ModuleDescriptors*          getModuleDescriptors() { return m_descriptorsModule; }
    ModuleTime*                 getModuleTime() { return m_timeModule; }
    ModuleUI*                   getModuleUI() { return m_uiModule; }
    ModuleRender*               getModuleRender() { return m_renderModule; }
    ModuleNavigation*           getModuleNavigation() { return m_navigationModule; }
    ModuleScene*                getModuleScene() { return m_sceneModule; }
    ModuleGameView*             getModuleGameView() { return m_gameViewModule; }
    ModuleFileSystem*           getModuleFileSystem() { return m_fileSystemModule; }
    ModuleAssets*               getAssetModule() { return m_assetsModule; }
    ModuleEventSystem*          getModuleEventSystem() { return m_moduleEventSystem; }

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
    ModuleD3D12*            m_d3d12Module = nullptr;
    ModuleEditor*           m_editorModule = nullptr;
    ModuleResources*        m_resourcesModule = nullptr;
    ModuleCamera*           m_cameraModule = nullptr;
    ModuleInput*            m_inputModule = nullptr;
    ModuleDescriptors*      m_descriptorsModule = nullptr;
    ModuleTime*             m_timeModule = nullptr;
    ModuleRender*           m_renderModule = nullptr;
    ModuleNavigation*       m_navigationModule = nullptr;
    ModuleScene*            m_sceneModule = nullptr;
    ModuleGameView*         m_gameViewModule = nullptr;
    ModuleFileSystem*       m_fileSystemModule = nullptr;
    ModuleAssets*           m_assetsModule = nullptr;
    ModuleUI*               m_uiModule = nullptr;
    ModuleEventSystem*      m_moduleEventSystem = nullptr;

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
