#pragma once

#include <vector>
#include <cstdint>
#include <windef.h>

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
class ModuleFlyweight;

class CameraComponent;
class Settings;

enum class ENGINE_STATE
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
	void         update();
	bool         cleanUp();

    ModuleD3D12*                getModuleD3D12() { return m_moduleD3d12M; }
    ModuleEditor*               getModuleEditor() { return m_moduleEditor; }
    ModuleResources*            getModuleResources() { return m_moduleResources; }
    ModuleInput*                getModuleInput() { return m_moduleInput; }
    ModuleCamera*               getModuleCamera() { return m_moduleCamera; }
    ModuleDescriptors*          getModuleDescriptors() { return m_moduleDescriptors; }
    ModuleTime*                 getModuleTime() { return m_moduleTime; }
    ModuleUI*                   getModuleUI() { return m_moduleUI; }
    ModuleRender*               getModuleRender() { return m_moduleRender; }
    ModuleNavigation*           getModuleNavigation() { return m_moduleNavigation; }
    ModuleScene*                getModuleScene() { return m_moduleScene; }
    ModuleGameView*             getModuleGameView() { return m_moduleGameView; }
    ModuleFileSystem*           getModuleFileSystem() { return m_moduleFileSystem; }
    ModuleAssets*               getModuleAssets() { return m_moduleAssets; }
    ModuleEventSystem*          getModuleEventSystem() { return m_eventSystemModule; }

    Settings*                   getSettings() { return m_settings; }

    // FIXME: Cannot return const CameraComponent* (which it should) because render is not const
    const CameraComponent* getCurrentCameraPerspective() const { return m_currentCameraPerspective; }
    void setCurrentCameraPerspective(CameraComponent* camera) { m_currentCameraPerspective = camera; }

	ENGINE_STATE getCurrentEngineState() const { return m_currentEngineState; }
	void setEngineState(int index) { m_currentEngineState = static_cast<ENGINE_STATE>(index); }

    bool        isPaused() const { return m_paused; }
    bool        setPaused(bool p) { m_paused = p; return m_paused; }

    void requestApplicationExit() { m_quit = true; }
    bool shouldQuit() const { return m_quit; }

    HWND getWindowHandle() const { return m_hWnd; }

    uint64_t                    getElapsedMilis() const { return m_elapsedMilis; }

private:

    std::vector<Module*>    modules;
    ModuleD3D12*            m_moduleD3d12M = nullptr;
    ModuleEditor*           m_moduleEditor = nullptr;
    ModuleResources*        m_moduleResources = nullptr;
    ModuleCamera*           m_moduleCamera = nullptr;
    ModuleInput*            m_moduleInput = nullptr;
    ModuleDescriptors*      m_moduleDescriptors = nullptr;
    ModuleTime*             m_moduleTime = nullptr;
    ModuleRender*           m_moduleRender = nullptr;
    ModuleNavigation*       m_moduleNavigation = nullptr;
    ModuleScene*            m_moduleScene = nullptr;
    ModuleGameView*         m_moduleGameView = nullptr;
    ModuleFileSystem*       m_moduleFileSystem = nullptr;
    ModuleAssets*           m_moduleAssets = nullptr;
    ModuleUI*               m_moduleUI = nullptr;
    ModuleEventSystem*      m_eventSystemModule = nullptr;

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
