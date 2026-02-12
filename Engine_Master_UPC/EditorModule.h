#pragma once
#include <Module.h>
#include "DescriptorHeap.h"
#include "EditorWindow.h"
#include "SceneEditor.h"
#include <vector>

class ImGuiPass;

class Logger;
class HardwareWindow;
class PerformanceWindow;
class SceneEditor;
class Hierarchy;
class DebugDrawPass;

class EditorModule: public Module
{
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

    bool m_showMainDockspace = true;
    bool m_firstFrame = true;
#pragma endregion

	GameObject* m_selectedGameObject;
};

