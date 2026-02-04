#pragma once
#include "EditorWindow.h"
#include "ImGuizmo.h"
#include <vector>
#include "CameraModule.h"

class Transform;
class DebugDrawPass;

class InputModule;

class SceneEditor: public EditorWindow
{
public:
    SceneEditor();
    const char* GetWindowName() const override { return "Scene Editor"; }
    void Update() override;
    void Render() override;
    bool Resize(ImVec2 contentRegion);

    void SetSelectedGameObject(Transform* transform) { this->transform = transform; }

    ImGuizmo::OPERATION GetCurrentOperation() const { return m_CurrentGizmoOperation; }
    ImGuizmo::MODE GetCurrentMode() const { return m_CurrentGizmoMode; }

    void RenderDebugDrawPass(ID3D12GraphicsCommandList* commandList);

    CameraCommand* CreateMovementCommand(CameraCommand::Type type, Keyboard::Keys key, const Vector3& direction);
    void BindCameraCommands();

private:
    //DebugDrawPass
    std::unique_ptr<DebugDrawPass> debugDrawPass;
    bool _showGrid = true;
    bool _showAxis = true;

    CameraModule* m_Camera;
    InputModule* m_Input;
    std::vector<CameraCommand*> m_CameraCommands;
    Transform* transform;

    ImGuizmo::OPERATION m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE m_CurrentGizmoMode = ImGuizmo::LOCAL;
};

