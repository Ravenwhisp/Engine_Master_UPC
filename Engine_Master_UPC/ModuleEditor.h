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

class ModuleGameView;
class WindowSceneEditor;
class WindowGame;
class EditorWindow;
class WindowGameDebug;
class GameObject;
class WindowAnimationStateMachine;

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

    void        setSelectedGameObject(GameObject* go) { m_selectedGameObject = go; }
    GameObject* getSelectedGameObject()         const { return m_selectedGameObject; }

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
    SCENE_TOOL      currentSceneTool = NONE;
    NAVIGATION_MODE currentNavigationMode = PAN;
    SCENE_TOOL      previousSceneTool = NONE;
    SIMULATION_MODE currentSimulationMode = STOP;
    GameObject* m_selectedGameObject = nullptr;
    bool            gizmoUseLocal = true;

    // Prefab editing
    PrefabEditSession m_prefabSession;
    bool              m_pendingExitPrefab = false;
};