#pragma once
#include <Module.h>
#include "Application.h"
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

	bool postInit();
	void update() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	bool cleanUp() override;

	SceneEditor*	getSceneEditor() { return m_sceneView; }
	ImVec2			getSceneEditorSize() { return m_sceneView->getSize();}
	ImGuiPass*		getImGuiPass() { return m_gui; }
private:
	void			setupDockLayout(ImGuiID dockspace_id);
	void			mainDockspace(bool* open);

	std::vector<EditorWindow*>	m_editorWindows;

	Logger*						m_logger = nullptr;
	HardwareWindow*				m_hardwareWindow = nullptr;
	PerformanceWindow*			m_performanceWindow = nullptr;
	SceneEditor*				m_sceneView = nullptr;
	ImGuiPass* 					m_gui = nullptr;
	DebugDrawPass*				m_debugDrawPass = nullptr;

    bool m_showMainDockspace = true;
    bool m_firstFrame = true;
};

