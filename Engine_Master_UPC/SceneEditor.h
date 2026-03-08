#pragma once
#include "EditorWindow.h"

#include "ImGuizmo.h"
#include <vector>

class InputModule;
class CameraModule;

class Settings;
class Quadtree;

class EditorToolbar;
class PlayToolbar;

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
	PlayToolbar* m_playToolbar;

    float m_viewportX = 0.0f;
    float m_viewportY = 0.0f;

public:
    SceneEditor();
    ~SceneEditor();

    const char* getWindowName() const override { return "Scene Editor"; }
    void        update() override;
    void        render() override;
    bool        resize(ImVec2 contentRegion);

    void renderDebugDrawPass(ID3D12GraphicsCommandList* commandList);
    void renderQuadtree();
    void drawBoundingBox(const Engine::BoundingBox& bbox, const ddVec3& color);


    float  getViewportX()      const { return m_viewportX; }
    float  getViewportY()      const { return m_viewportY; }

};
