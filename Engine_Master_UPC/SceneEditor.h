#pragma once
#include "EditorWindow.h"

#include "ImGuizmo.h"
#include <vector>

#include "CameraModule.h"
;
class InputModule;

class Settings;
class Quadtree;

class DebugDrawPass;
class GameObject;

class SceneEditor: public EditorWindow
{
private:
    InputModule* m_inputModule;
    CameraModule* m_cameraModule;

    Settings* m_settings;
    Quadtree* m_quadtree;

public:
    SceneEditor();
    const char* getWindowName() const override { return "Scene Editor"; }
    void        update() override;
    void        render() override;
    bool        resize(ImVec2 contentRegion);

    ImGuizmo::OPERATION getCurrentOperation() const { return m_currentGizmoOperation; }
    ImGuizmo::MODE      getCurrentMode() const { return m_currentGizmoMode; }

    void renderDebugDrawPass(ID3D12GraphicsCommandList* commandList);
    void renderQuadtree();

private:
    CameraCommand* createMovementCommand(CameraCommand::Type type, Keyboard::Keys key, const Vector3& direction);
    void bindCameraCommands();

    std::unique_ptr<DebugDrawPass>  m_debugDrawPass;
    std::vector<CameraCommand*>     m_cameraCommands;

    ImGuizmo::OPERATION m_currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE      m_currentGizmoMode = ImGuizmo::LOCAL;
};

