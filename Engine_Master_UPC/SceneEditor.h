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

class SceneEditor: public EditorWindow
{
private:
    InputModule* m_inputModule;
    CameraModule* m_cameraModule;

    Settings* m_settings;
    Quadtree* m_quadtree;

    EditorToolbar* m_editorToolbar;

    ImVec2 m_viewportPos{ 0,0 };
    ImVec2 m_viewportSize{ 0,0 };

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
};

