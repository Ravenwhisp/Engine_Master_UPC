#pragma once
#include <Module.h>
#include "DescriptorHeap.h"
#include "EditorWindow.h"
#include "SceneEditor.h"
#include <vector>
#include "Keyboard.h"
#include "Mouse.h"

class ImGuiPass;

class Logger;
class HardwareWindow;
class PerformanceWindow;
class SceneEditor;
class Hierarchy;
class DebugDrawPass;
class EditorSettings;
class SceneConfig;
class GameWindow;

class EditorModule: public Module
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
	EditorModule();
	~EditorModule() {}

#pragma region Game Loop
	bool init() override;
	void update() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	bool cleanUp() override;
#pragma endregion

	SceneEditor*	getSceneEditor() { return m_sceneEditor; }
	ImVec2			getSceneEditorSize() { return m_sceneEditor->getSize();}
	ImGuiPass*		getImGuiPass() { return m_gui; }

	void			setSelectedGameObject(GameObject* selectedGameObject) { m_selectedGameObject = selectedGameObject; }
	GameObject*		getSelectedGameObject() { return m_selectedGameObject; }

	SCENE_TOOL		getCurrentSceneTool() const { return currentSceneTool; }
	NAVIGATION_MODE getCurrentNavigationMode() const { return currentNavigationMode; }
	SIMULATION_MODE	getCurrentSceneMode() const { return currentSimulationMode; }

	void			setCurrentSceneTool(int tool) { currentSceneTool = static_cast<SCENE_TOOL>(tool); }
	void			setCurrentSimulationMode(int mode) { currentSimulationMode = static_cast<SIMULATION_MODE>(mode); }
	bool			isGizmoLocal() const { return gizmoUseLocal; }
	void			toggleGizmoMode() { gizmoUseLocal = !gizmoUseLocal; }

private:
	void			setupDockLayout(ImGuiID dockspace_id);
	void			mainDockspace(bool* open);

private:
#pragma region Views
	std::vector<EditorWindow*>	m_editorWindows;
	Logger*						m_logger = nullptr;
	HardwareWindow*				m_hardwareWindow = nullptr;
	PerformanceWindow*			m_performanceWindow = nullptr;
	SceneEditor*				m_sceneEditor = nullptr;
	ImGuiPass* 					m_gui = nullptr;
	DebugDrawPass*				m_debugDrawPass = nullptr;
	EditorSettings*				m_editorSettings = nullptr;
	SceneConfig*				m_sceneConfig = nullptr;
	GameWindow*					m_gameWindow = nullptr;

    bool m_showMainDockspace = true;
    bool m_firstFrame = true;
#pragma endregion

#pragma region Editor
	void setSceneTool(SCENE_TOOL newTool);
	void setMode(SCENE_TOOL sceneTool, NAVIGATION_MODE navigationMode);
	void setSimulationMode(SIMULATION_MODE newMode);
	void resetMode();

	void handleKeyboardShortcuts();
	void handleQWERTYCases(Keyboard::State keyboardState);

	SCENE_TOOL currentSceneTool;
	NAVIGATION_MODE currentNavigationMode;
	SCENE_TOOL previousSceneTool;
	SIMULATION_MODE currentSimulationMode = STOP;

	GameObject* m_selectedGameObject;
	bool gizmoUseLocal = true;
#pragma endregion

	void ConsoleLog(const char* message);
	void ConsoleWarn(const char* message);
	void ConsoleError(const char* message);
};

