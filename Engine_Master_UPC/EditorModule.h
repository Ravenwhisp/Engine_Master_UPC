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
	~EditorModule();

	bool postInit();
	void update() override;
	void preRender() override;
	void render() override;
	void postRender() override;
	bool cleanUp() override;

	SceneEditor* GetSceneEditor() { return _sceneView; }
	ImVec2 GetSceneEditorSize() { return _sceneView->GetSize();}
	ImGuiPass* GetImGuiPass() { return _gui; }
private:
	void SetupDockLayout(ImGuiID dockspace_id);
	void MainDockspace(bool* p_open);

	std::vector<EditorWindow*> _editorWindows;
	Logger* _logger = nullptr;
	HardwareWindow* _hardwareWindow = nullptr;
	PerformanceWindow* _performanceWindow = nullptr;
	SceneEditor* _sceneView = nullptr;

	ImGuiPass* _gui = nullptr;

	DebugDrawPass* _debugDrawPass = nullptr;
    bool _showMainDockspace = true;

    bool _firstFrame = true;
};

