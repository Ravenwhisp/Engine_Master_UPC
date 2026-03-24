#pragma once
#include <Module.h>
#include "PrefabEditSession.h"
#include "Keyboard.h"

class ModuleGameView;

class WindowLogger;
class WindowHardware;
class WindowPerformance;
class WindowSceneEditor;
class WindowHierarchy;
class WindowEditorSettings;
class WindowGame;
class WindowSceneEditor;
class EditorWindow;
class WindowGameDebug;

class SceneConfig;
struct PrefabEditSession;


class ModuleEditor: public Module
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
	bool init() override;
	void update() override;
	void render() override;
	bool cleanUp() override;
#pragma endregion

	WindowSceneEditor*	getWindowSceneEditor() { return m_sceneEditor; }

	ImVec2 getEventViewport() const;
	ImVec2 getEventViewportSize() const;


	WindowGame*		getWindowGame() { return m_gameWindow; }

	void			setSelectedGameObject(GameObject* selectedGameObject) { m_selectedGameObject = selectedGameObject; }
	GameObject*		getSelectedGameObject() const { return m_selectedGameObject; }

	void enterPrefabEdit(const std::filesystem::path& sourcePath);
	void               exitPrefabEdit();
	PrefabEditSession* getPrefabSession() { return &m_prefabSession; }

	SCENE_TOOL		getCurrentSceneTool() const { return currentSceneTool; }
	NAVIGATION_MODE getCurrentNavigationMode() const { return currentNavigationMode; }
	SIMULATION_MODE	getCurrentSimulationMode() const { return currentSimulationMode; }

	void			setCurrentSceneTool(int tool) { currentSceneTool = static_cast<SCENE_TOOL>(tool); }
	void			setCurrentSimulationMode(int mode) { currentSimulationMode = static_cast<SIMULATION_MODE>(mode); }
	bool			isGizmoLocal() const { return gizmoUseLocal; }
	void			toggleGizmoMode() { gizmoUseLocal = !gizmoUseLocal; }

private:
	void			setupDockLayout(ImGuiID dockspace_id);
	void			mainDockspace(bool* open);

	ModuleGameView* m_moduleGameView;

#pragma region Views
	std::vector<EditorWindow*>	m_editorWindows;
	WindowLogger*						m_logger = nullptr;
	WindowHardware*				m_hardwareWindow = nullptr;
	WindowPerformance*			m_performanceWindow = nullptr;
	WindowSceneEditor*				m_sceneEditor = nullptr;
	WindowEditorSettings*				m_editorSettings = nullptr;
	SceneConfig*				m_sceneConfig = nullptr;
	WindowGame*					m_gameWindow = nullptr;

    bool m_showMainDockspace = true;
    bool m_firstFrame = true;

	std::unique_ptr<WindowGameDebug> m_viewGameDebug;
#pragma endregion

#pragma region Editor
	void setSceneTool(SCENE_TOOL newTool);
	void setMode(SCENE_TOOL sceneTool, NAVIGATION_MODE navigationMode);
	void setSimulationMode(SIMULATION_MODE newMode);
	void resetMode();

	void handleKeyboardShortcuts();
	void handleQWERTYCases(Keyboard::State keyboardState);
	void flushExitPrefabEdit();

	SCENE_TOOL currentSceneTool;
	NAVIGATION_MODE currentNavigationMode;
	SCENE_TOOL previousSceneTool;
	SIMULATION_MODE currentSimulationMode = STOP;

	GameObject* m_selectedGameObject = nullptr;
	bool gizmoUseLocal = true;
#pragma endregion

#pragma region Prefab Editing
	PrefabEditSession m_prefabSession;
	bool              m_pendingExitPrefab = false;
#pragma endregion
};

