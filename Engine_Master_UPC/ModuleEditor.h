#pragma once
#include <Module.h>
#include "PrefabEditSession.h"
#include "Keyboard.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include "mutex"

class ModuleGameView;
class WindowSceneEditor;
class WindowGame;
class EditorWindow;
class WindowGameDebug;
class WindowAnimationStateMachine;

class GameObject;
class Asset;

class ModuleEditor : public Module
{

public:

    enum SCENE_TOOL
    {
        NONE = -1,
        NAVIGATION = 0,
        MOVE = 1,
        ROTATE = 2,
        SCALE = 3,
        RECT = 4,
        TRANSFORM = 5
    };

    enum NAVIGATION_MODE
    {
        PAN = 0,
        ORBIT = 1,
        ZOOM = 2,
        FREE_LOOK = 3
    };

    enum SIMULATION_MODE
    {
        PLAY = 0,
        PAUSE = 1,
        STOP = 2
    };

public:

    ModuleEditor();
    ~ModuleEditor();

#pragma region Game Loop

    bool init()    override;
    void update()  override;
    void render()  override;
    bool cleanUp() override;

#pragma endregion

    template<class T>
    void registerWindowType(const char* typeKey)
    {
        m_windowFactories[typeKey] = [typeKey]() -> EditorWindow*
            {
                return new T();
            };
    }

    EditorWindow* openWindow(const std::string& typeKey);

    template<class T>
    T* findWindow() const
    {
        for (EditorWindow* w : m_editorWindows)
        {
            if (T* typed = dynamic_cast<T*>(w))
            {
                return typed;
            }
        }
        return nullptr;
    }

    template<class T>
    std::vector<T*> findAllWindows() const
    {
        std::vector<T*> result;
        for (EditorWindow* w : m_editorWindows)
        {
            if (T* typed = dynamic_cast<T*>(w))
            {
                result.push_back(typed);
            }
        }
        return result;
    }

    WindowSceneEditor* getWindowSceneEditor() const;
    WindowGame* getWindowGame()                 const;
    WindowAnimationStateMachine* getWindowAnimationStateMachine() const;

    ImVec2 getEventViewport()     const;
    ImVec2 getEventViewportSize() const;

    void setSelectedGameObject(GameObject* go) { m_selectedGameObject = go; m_selectedAsset = nullptr; }
    GameObject* getSelectedGameObject() const { return m_selectedGameObject; }
    void setSelectedAsset(std::shared_ptr<Asset> asset) { m_selectedAsset = asset; m_selectedGameObject = nullptr; }
    std::shared_ptr<Asset> getSelectedAsset() const { return m_selectedAsset; }

    bool isInPrefabEditMode() const { return m_prefabSession.m_active && m_prefabSession.m_rootObject != nullptr; }
    GameObject* getPrefabEditRoot() const { return isInPrefabEditMode() ? m_prefabSession.m_rootObject : nullptr; }
    const std::filesystem::path& getPrefabEditSourcePath() const { return m_prefabSession.m_sourcePath; }
    Scene* getPrefabEditScene() const { return isInPrefabEditMode() ? m_prefabSession.m_isolatedScene : nullptr; }

    void              enterPrefabEdit(const std::filesystem::path& sourcePath);
    void              exitPrefabEdit();
    PrefabEditSession* getPrefabSession() { return &m_prefabSession; }


    SCENE_TOOL      getCurrentSceneTool()      const { return currentSceneTool; }
    NAVIGATION_MODE getCurrentNavigationMode() const { return currentNavigationMode; }
    SIMULATION_MODE getCurrentSimulationMode() const { return currentSimulationMode; }

    void setCurrentSceneTool(int tool)
    {
        currentSceneTool = static_cast<SCENE_TOOL>(tool);
    }

    void setCurrentSimulationMode(int mode)
    {
        currentSimulationMode = static_cast<SIMULATION_MODE>(mode);
    }

    bool isGizmoLocal()  const { return gizmoUseLocal; }
    void toggleGizmoMode() { gizmoUseLocal = !gizmoUseLocal; }

    void mainMenuBar();

private:

    void saveWindowStates();
    void loadWindowStates();
    void removeClosedWindows();
    void setupDockLayout(ImGuiID dockspace_id);
    void mainDockspace(bool* open);

    void setSceneTool(SCENE_TOOL newTool);
    void setMode(SCENE_TOOL sceneTool, NAVIGATION_MODE navigationMode);
    void resetMode();
    void handleKeyboardShortcuts();
    void handleQWERTYCases(Keyboard::State keyboardState);
    void flushExitPrefabEdit();

    ModuleGameView* m_moduleGameView = nullptr;

    std::vector<EditorWindow*>   m_editorWindows;

    std::unordered_map<std::string, std::function<EditorWindow* ()>> m_windowFactories;

    int m_nextInstanceId = 1;

    bool m_showMainDockspace = true;
    bool m_firstFrame = true;

    std::unique_ptr<WindowGameDebug> m_viewGameDebug;

    // Editor state
    SCENE_TOOL currentSceneTool = NONE;
    NAVIGATION_MODE currentNavigationMode = PAN;
    SCENE_TOOL previousSceneTool = NONE;
    SIMULATION_MODE currentSimulationMode = STOP;
    bool gizmoUseLocal = true;

    GameObject* m_selectedGameObject = nullptr;
    std::shared_ptr<Asset> m_selectedAsset = nullptr;

    // Prefab editing
    PrefabEditSession m_prefabSession;
    bool              m_pendingExitPrefab = false;

    // Async file dialog for DataContainer export/import
    std::atomic<bool>      m_dcDialogRunning = false;
    std::mutex             m_dcDialogMutex;
    std::optional<std::filesystem::path> m_dcDialogResult;
    int                    m_dcDialogMode = 0; // 1=export, 2=import
};