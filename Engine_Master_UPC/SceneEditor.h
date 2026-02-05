#pragma once
#include "EditorWindow.h"
#include "ImGuizmo.h"
#include <vector>
#include "CameraModule.h"

class GameObject;
class DebugDrawPass;

class InputModule;

class SceneEditor: public EditorWindow
{
public:
    SceneEditor();
    const char* getWindowName() const override { return "Scene Editor"; }
    void        update() override;
    void        render() override;
    bool        resize(ImVec2 contentRegion);

    void setSelectedGameObject(GameObject* gameObject) { m_selectedGameObject = gameObject; }

    ImGuizmo::OPERATION getCurrentOperation() const { return m_currentGizmoOperation; }
    ImGuizmo::MODE      getCurrentMode() const { return m_currentGizmoMode; }

    void renderDebugDrawPass(ID3D12GraphicsCommandList* commandList);

private:

    CameraCommand* createMovementCommand(CameraCommand::Type type, Keyboard::Keys key, const Vector3& direction);
    void bindCameraCommands();

    std::unique_ptr<DebugDrawPass>  m_debugDrawPass;
    CameraModule*                   m_camera;
    InputModule*                    m_input;
    std::vector<CameraCommand*>     m_cameraCommands;
    GameObject*                     m_selectedGameObject;

    bool m_showGrid = true;
    bool m_showAxis = true;

    ImGuizmo::OPERATION m_currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE      m_currentGizmoMode = ImGuizmo::LOCAL;
};

