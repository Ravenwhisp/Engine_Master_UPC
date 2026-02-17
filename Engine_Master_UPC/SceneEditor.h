#pragma once
#include "EditorWindow.h"

#include "ImGuizmo.h"
#include <vector>

class InputModule;
class CameraModule;

class Settings;
class Quadtree;

class EditorToolbar;

class DebugDrawPass;
class GameObject;
class SceneSerializer;

class SceneEditor: public EditorWindow
{
private:
    InputModule* m_inputModule;
    CameraModule* m_cameraModule;

    Settings* m_settings;
    Quadtree* m_quadtree;

    EditorToolbar* m_editorToolbar;
	SceneSerializer* m_sceneSerializer;

private:
    std::unique_ptr<DebugDrawPass>  m_debugDrawPass;

public:
    SceneEditor();
    ~SceneEditor();

    const char* getWindowName() const override { return "Scene Editor"; }
    void        update() override;
    void        render() override;
    bool        resize(ImVec2 contentRegion);

    void renderDebugDrawPass(ID3D12GraphicsCommandList* commandList);
    void renderQuadtree();

#pragma region Persistence
    void saveScene();
	void loadScene();
#pragma endregion

};

