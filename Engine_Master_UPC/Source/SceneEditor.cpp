#include "Globals.h"
#include "SceneEditor.h"
#include "ImGuizmo.h"
#include <imgui.h>
#include "Transform.h"

#include "Application.h"
#include "D3D12Module.h"
#include "InputModule.h"
#include "TimeModule.h"
#include "RenderModule.h"
#include "DebugDrawPass.h"

SceneEditor::SceneEditor()
{
    m_Camera = app->GetCameraModule();
    m_Input = app->GetInputModule();
    auto d3d12Module = app->GetD3D12Module();
    debugDrawPass = std::make_unique<DebugDrawPass>(d3d12Module->GetDevice(), d3d12Module->GetCommandQueue()->GetD3D12CommandQueue().Get(), false);

    BindCameraCommands();
}

void SceneEditor::Update()
{
    if (!IsFocused()) {
        return;
    }

    // Execute all camera commands
    float deltaTime = app->GetTimeModule()->deltaTime();
    for (const auto& command : m_CameraCommands) {
        command->Execute(m_Camera, deltaTime);
    }
}

void SceneEditor::Render()
{
    if (!ImGui::Begin(GetWindowName(), GetOpenPtr(),
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    windowX = windowPos.x;
    windowY = windowPos.y;

    // Get available content region for the scene view
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    // Only resize if we have valid dimensions
    if (contentRegion.x > 0 && contentRegion.y > 0) {
        Resize(contentRegion);
        ImTextureID textureID = (ImTextureID)app->GetRenderModule()->GetGPUScreenRT().ptr;
        ImGui::Image(textureID, m_Size);
        
    }

    // Setup must happen AFTER ImGui::Image() to be on top
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    // Get the content region coordinates (excluding scrollbars)
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 contentPos(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 contentSize(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

    ImGuizmo::SetRect(contentPos.x, contentPos.y, contentSize.x, contentSize.y);
    ImGuizmo::Enable(true);

    if (ImGui::IsKeyPressed(ImGuiKey_T))
            m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_E))
            m_CurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R))
            m_CurrentGizmoOperation = ImGuizmo::SCALE;

    if (transform && m_Camera && ImGui::IsWindowHovered())
    {
        auto modelMatrix = transform->GetWorldMatrix();

        // Apply the gizmo manipulation
        ImGuizmo::Manipulate(
            (float*)&m_Camera->GetViewMatrix(),
            (float*)&m_Camera->GetProjectionMatrix(),
            m_CurrentGizmoOperation,
            m_CurrentGizmoMode,
            (float*)&modelMatrix,
            NULL,
            NULL
        );

        // If gizmo was used, update the model
        if (ImGuizmo::IsUsing())
        {
            transform->SetWorldMatrix(modelMatrix);
        }
    }

    // Update viewport hover/focus state
    _isViewportHovered = ImGui::IsWindowHovered();
    _isViewportFocused = ImGui::IsWindowFocused();

    ImGui::End();
}

bool SceneEditor::Resize(ImVec2 contentRegion)
{
    if (abs(contentRegion.x - m_Size.x) > 1.0f ||
        abs(contentRegion.y - m_Size.y) > 1.0f) {
        SetSize(contentRegion);
        return true;
    }
    return false;
}

void SceneEditor::RenderDebugDrawPass(ID3D12GraphicsCommandList* commandList)
{
    if (_showGrid) {
        dd::xzSquareGrid(-10.0f, 10.f, 0.0f, 1.0f, dd::colors::LightGray);
    }

    debugDrawPass->record(commandList, GetSize().x, GetSize().y, m_Camera->GetViewMatrix(), m_Camera->GetProjectionMatrix());
}



CameraCommand* SceneEditor::CreateMovementCommand(CameraCommand::Type type, Keyboard::Keys key, const Vector3& direction)
{
    CameraCommand* command = new CameraCommand(
        type,
        [this, key]() {
            return m_Input->IsRightMouseDown() && m_Input->IsKeyDown(key);
        },
        [this, direction](CameraModule* camera, float deltaTime) {
            float speed = camera->GetSpeed() * deltaTime;
            if (m_Input->IsKeyDown(Keyboard::LeftShift)) {
                speed *= 2.0f;
            }
            Vector3 moveDir = direction.x * camera->GetRight() + direction.y * camera->GetUp() + direction.z * camera->GetForward();
            camera->Move(moveDir * speed);
        }
    );
    return command;
}

void SceneEditor::BindCameraCommands()
{
    m_CameraCommands.clear();

    //MOVE COMMANDS
    m_CameraCommands.emplace_back(CreateMovementCommand(CameraCommand::MOVE_FORWARD, Keyboard::W, Vector3(0, 0, 1)));
    m_CameraCommands.emplace_back(CreateMovementCommand(CameraCommand::MOVE_BACKWARD, Keyboard::S, Vector3(0, 0, -1)));
    m_CameraCommands.emplace_back(CreateMovementCommand(CameraCommand::MOVE_LEFT, Keyboard::A, Vector3(-1, 0, 0)));
    m_CameraCommands.emplace_back(CreateMovementCommand(CameraCommand::MOVE_RIGHT, Keyboard::D, Vector3(1, 0, 0)));
    m_CameraCommands.emplace_back(CreateMovementCommand(CameraCommand::MOVE_UP, Keyboard::Q, Vector3(0, 1, 0)));
    m_CameraCommands.emplace_back(CreateMovementCommand(CameraCommand::MOVE_DOWN, Keyboard::E, Vector3(0, -1, 0)));

    //ZOOM COMMAND
    m_CameraCommands.emplace_back(new CameraCommand(
        CameraCommand::ZOOM,
        [this]() {
            return true;
        },
        [this](CameraModule* camera, float deltaTime) {
            float wheel = 0.0f;
            m_Input->GetMouseWheel(wheel);
            float zoomAmount = wheel * camera->GetSpeed() * deltaTime;
            camera->Zoom(zoomAmount);
        }));

    //FOCUS COMMAND
    m_CameraCommands.emplace_back(new CameraCommand(
        CameraCommand::FOCUS,
        [this]() {
            return m_Input->IsKeyDown(Keyboard::F);
        },
        [this](CameraModule* camera, float deltaTime) {
            if (transform) {
                //TODO: Move the camera near the object
                camera->Focus(camera->GetPosition(), transform->GetPosition());
            }
        }));
    
    //ORBIT COMMAND
    m_CameraCommands.emplace_back(new CameraCommand(
        CameraCommand::ORBIT,
        [this]() {
            return m_Input->IsKeyDown(Keyboard::LeftAlt) &&
                m_Input->IsLeftMouseDown();
        },
        [this](CameraModule* camera, float deltaTime) {
            float deltaX = 0.0f, deltaY = 0.0f;
            m_Input->GetMouseDelta(deltaX, deltaY);

            Quaternion yaw = Quaternion::CreateFromAxisAngle(camera->GetUp(), -deltaX * camera->GetSensitivity());
            Quaternion pitch = Quaternion::CreateFromAxisAngle(camera->GetRight(), -deltaY * camera->GetSensitivity());
            camera->Orbit(yaw * pitch, camera->GetTarget());
        }));

    //LOOK AROUND COMMAND
    m_CameraCommands.emplace_back(new CameraCommand(
        CameraCommand::LOOK,
        [this]() {
            return m_Input->IsRightMouseDown() && !m_Input->IsKeyDown(Keyboard::LeftAlt);
        },
        [this](CameraModule* camera, float deltaTime) {
            float deltaX = 0.0f, deltaY = 0.0f;
            m_Input->GetMouseDelta(deltaX, deltaY);

            Quaternion yaw = Quaternion::CreateFromAxisAngle(camera->GetUp(), deltaX * camera->GetSensitivity());
            Quaternion pitch = Quaternion::CreateFromAxisAngle(camera->GetRight(), deltaY * camera->GetSensitivity());
            camera->Rotate(yaw * pitch);
        }));
}
