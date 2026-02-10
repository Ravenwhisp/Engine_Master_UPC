#include "Globals.h"
#include "SceneEditor.h"
#include "ImGuizmo.h"
#include <imgui.h>

#include "Application.h"
#include "D3D12Module.h"
#include "InputModule.h"
#include "TimeModule.h"
#include "RenderModule.h"
#include "DebugDrawPass.h"

#include "GameObject.h"

SceneEditor::SceneEditor()
{
    m_camera = app->getCameraModule();
    m_input = app->getInputModule();
    auto d3d12Module = app->getD3D12Module();
    m_debugDrawPass = std::make_unique<DebugDrawPass>(d3d12Module->getDevice(), d3d12Module->getCommandQueue()->getD3D12CommandQueue().Get(), false);

    bindCameraCommands();
}

void SceneEditor::update()
{
    if (!isFocused()) return;

    // Execute all camera commands
    float deltaTime = app->getTimeModule()->deltaTime();
    for (const auto& command : m_cameraCommands) 
    {
        command->Execute(m_camera, deltaTime);
    }
}

void SceneEditor::render()
{
    if (!ImGui::Begin(getWindowName(), getOpenPtr(), ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::End();
        return;
    }

    ImVec2 windowPos = ImGui::GetWindowPos();
    m_windowX = windowPos.x;
    m_windowY = windowPos.y;

    ImVec2 contentRegion = ImGui::GetContentRegionAvail();

    if (contentRegion.x > 0 && contentRegion.y > 0) 
    {
        resize(contentRegion);
        ImTextureID textureID = (ImTextureID)app->getRenderModule()->getGPUScreenRT().ptr;
        ImGui::Image(textureID, m_size);
        
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 contentPos(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 contentSize(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

    ImGuizmo::SetRect(contentPos.x, contentPos.y, contentSize.x, contentSize.y);
    ImGuizmo::Enable(true);

    if (ImGui::IsKeyPressed(ImGuiKey_T)) 
    {
        m_currentGizmoOperation = ImGuizmo::TRANSLATE;

    }
    else if (ImGui::IsKeyPressed(ImGuiKey_E)) 
    {
        m_currentGizmoOperation = ImGuizmo::ROTATE;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_R))
    {
        m_currentGizmoOperation = ImGuizmo::SCALE;
    }

    if (m_selectedGameObject && m_camera)
    {
        Transform* transform = m_selectedGameObject->GetTransform();

        Matrix worldMatrix = transform->getGlobalMatrix();

        ImGuizmo::Manipulate(
            (float*)&m_camera->getViewMatrix(),
            (float*)&m_camera->getProjectionMatrix(),
            m_currentGizmoOperation,
            m_currentGizmoMode,
            (float*)&worldMatrix
        );

        if (ImGuizmo::IsUsing())
        {
            Matrix localMatrix = worldMatrix;

            Transform* parent = transform->getRoot();
            if (parent)
            {
                Matrix parentWorldInv = parent->getGlobalMatrix().Invert();
                localMatrix = worldMatrix * parentWorldInv;
            }

            float translation[3];
            float rotation[3];
            float scale[3];

            ImGuizmo::DecomposeMatrixToComponents(
                (float*)&localMatrix,
                translation,
                rotation,
                scale
            );

            transform->setPosition(Vector3(translation[0], translation[1], translation[2]));
            transform->setRotationEuler(Vector3(rotation[0], rotation[1], rotation[2]));
            transform->setScale(Vector3(scale[0], scale[1], scale[2]));
        }
    }


    m_isViewportHovered = ImGui::IsWindowHovered();
    m_isViewportFocused = ImGui::IsWindowFocused();

    ImGui::End();
}

bool SceneEditor::resize(ImVec2 contentRegion)
{
    if (abs(contentRegion.x - m_size.x) > 1.0f ||
        abs(contentRegion.y - m_size.y) > 1.0f) {
        setSize(contentRegion);
        return true;
    }
    return false;
}

void SceneEditor::renderDebugDrawPass(ID3D12GraphicsCommandList* commandList)
{
    if (m_showGrid) 
    {
        dd::xzSquareGrid(-10.0f, 10.f, 0.0f, 1.0f, dd::colors::LightGray);
    }

    m_debugDrawPass->record(commandList, getSize().x, getSize().y, m_camera->getViewMatrix(), m_camera->getProjectionMatrix());
}



CameraCommand* SceneEditor::createMovementCommand(CameraCommand::Type type, Keyboard::Keys key, const Vector3& direction)
{
    CameraCommand* command = new CameraCommand(
        type,
        [this, key]() {
            return m_input->isRightMouseDown() && m_input->isKeyDown(key);
        },
        [this, direction](CameraModule* camera, float deltaTime) {
            float speed = camera->getSpeed() * deltaTime;
            if (m_input->isKeyDown(Keyboard::LeftShift)) {
                speed *= 2.0f;
            }
            Vector3 moveDir = direction.x * camera->getRight() + direction.y * camera->getUp() + direction.z * camera->getForward();
            camera->move(moveDir * speed);
        }
    );
    return command;
}

void SceneEditor::bindCameraCommands()
{
    m_cameraCommands.clear();

    m_cameraCommands.emplace_back(createMovementCommand(CameraCommand::MOVE_FORWARD, Keyboard::W, Vector3(0, 0, 1)));
    m_cameraCommands.emplace_back(createMovementCommand(CameraCommand::MOVE_BACKWARD, Keyboard::S, Vector3(0, 0, -1)));
    m_cameraCommands.emplace_back(createMovementCommand(CameraCommand::MOVE_LEFT, Keyboard::A, Vector3(-1, 0, 0)));
    m_cameraCommands.emplace_back(createMovementCommand(CameraCommand::MOVE_RIGHT, Keyboard::D, Vector3(1, 0, 0)));
    m_cameraCommands.emplace_back(createMovementCommand(CameraCommand::MOVE_UP, Keyboard::Q, Vector3(0, 1, 0)));
    m_cameraCommands.emplace_back(createMovementCommand(CameraCommand::MOVE_DOWN, Keyboard::E, Vector3(0, -1, 0)));


    m_cameraCommands.emplace_back(new CameraCommand(
        CameraCommand::ZOOM,
        [this]() {
            return true;
        },
        [this](CameraModule* camera, float deltaTime) {
            float wheel = 0.0f;
            m_input->getMouseWheel(wheel);
            float zoomAmount = wheel * camera->getSpeed() * deltaTime;
            camera->zoom(zoomAmount);
        }));


    m_cameraCommands.emplace_back(new CameraCommand(
        CameraCommand::FOCUS,
        [this]() {
            return m_input->isKeyDown(Keyboard::F);
        },
        [this](CameraModule* camera, float deltaTime) {
            if (m_selectedGameObject) {
                //TODO: Move the camera near the object
				const DirectX::SimpleMath::Vector3 objPos = * m_selectedGameObject->GetTransform()->getPosition();
                camera->focus(camera->getPosition(), objPos);
            }
        }));
    

    m_cameraCommands.emplace_back(new CameraCommand(
        CameraCommand::ORBIT,
        [this]() {
            return m_input->isKeyDown(Keyboard::LeftAlt) && m_input->isLeftMouseDown();
        },
        [this](CameraModule* camera, float deltaTime) {
            float deltaX = 0.0f, deltaY = 0.0f;
            m_input->getMouseDelta(deltaX, deltaY);

            Quaternion yaw = Quaternion::CreateFromAxisAngle(camera->getUp(), -deltaX * camera->getSensitivity());
            Quaternion pitch = Quaternion::CreateFromAxisAngle(camera->getRight(), -deltaY * camera->getSensitivity());
            camera->orbit(yaw * pitch, camera->getTarget());
        }));


    m_cameraCommands.emplace_back(new CameraCommand(
        CameraCommand::LOOK,
        [this]() {
            return m_input->isRightMouseDown() && !m_input->isKeyDown(Keyboard::LeftAlt);
        },
        [this](CameraModule* camera, float deltaTime) {
            float deltaX = 0.0f, deltaY = 0.0f;
            m_input->getMouseDelta(deltaX, deltaY);

            Quaternion yaw = Quaternion::CreateFromAxisAngle(camera->getUp(), deltaX * camera->getSensitivity());
            Quaternion pitch = Quaternion::CreateFromAxisAngle(camera->getRight(), deltaY * camera->getSensitivity());
            camera->rotate(yaw * pitch);
        }));
}
