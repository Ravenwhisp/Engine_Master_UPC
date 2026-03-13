#include "Globals.h"
#include "CameraModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "EditorModule.h"

#include "Settings.h"

#include "Mouse.h"
#include "Keyboard.h"

#include <algorithm>
#include <cmath>

#include "ImGuizmo.h"
#include "GameObject.h"

bool CameraModule::init() {
    m_settings = app->getSettings();

    m_inputModule = app->getInputModule();
    m_D3D12Module = app->getD3D12Module();
    m_editorModule = app->getEditorModule();

    syncYawPitchFromLookAt();
    syncDistanceFromLookAt();

    Mouse::State mouseState = Mouse::Get().GetState();
    m_lastWheel = mouseState.scrollWheelValue;

    rebuildViewProj();

    return true;
}


void CameraModule::update() {
    if (!app->getEditorModule()->getSceneEditor()->isFocused() || !app->getEditorModule()->getSceneEditor()->isHovered()) {
        m_lastWheel = Mouse::Get().GetState().scrollWheelValue;
        return;
    }
    ImVec2 sceneSize = m_editorModule->getSceneEditor()->getSize();
    if (sceneSize.x <= 1.0f || sceneSize.y <= 1.0f)
        return;

    unsigned width = (unsigned)sceneSize.x;
    unsigned height = (unsigned)sceneSize.y;

    float dt = app->getElapsedMilis() * 0.001f;

    int navMode = -1;
    if (m_editorModule->getCurrentSceneTool() == EditorModule::SCENE_TOOL::NAVIGATION)
        navMode = (int)m_editorModule->getCurrentNavigationMode();

    if (navMode != m_lastNavMode) {
        if (m_lastNavMode == (int)EditorModule::NAVIGATION_MODE::FREE_LOOK) {
            Vector3 forward = getForwardFromYawPitch();
            m_distanceToTarget = std::clamp(m_distanceToTarget,
                m_settings->camera.minDistance,
                m_settings->camera.maxDistance);

            m_target = m_position + forward * m_distanceToTarget;
            syncDistanceFromLookAt();
        }

        m_panFirst = m_orbitFirst = m_zoomFirst = m_flyFirst = true;
        m_lastNavMode = navMode;
    }

    bool allowCameraInput = m_editorModule->getSceneEditor()->isFocused() && !ImGuizmo::IsUsing();

    if (allowCameraInput) {
        if (m_editorModule->getCurrentSceneTool() == EditorModule::SCENE_TOOL::NAVIGATION) {
            switch (m_editorModule->getCurrentNavigationMode()) {
            case EditorModule::NAVIGATION_MODE::PAN:        panMode(); break;
            case EditorModule::NAVIGATION_MODE::ORBIT:      orbitMode(); break;
            case EditorModule::NAVIGATION_MODE::ZOOM:       zoomMode(); break;
            case EditorModule::NAVIGATION_MODE::FREE_LOOK:  flythroughMode(dt); break;
            }
        }

        handleAutoFocus();
        handleMouseWheel();
    }
    else {
        m_lastWheel = Mouse::Get().GetState().scrollWheelValue;
    }

    Vector3 viewTarget = m_target;
    if (m_editorModule->getCurrentNavigationMode() == EditorModule::NAVIGATION_MODE::FREE_LOOK)
        viewTarget = m_position + getForwardFromYawPitch();

    m_view = Matrix::CreateLookAt(m_position, viewTarget, m_up);
    m_projection = Matrix::CreatePerspectiveFieldOfView(m_fovY, (float)width / (float)height, m_nearPlane, m_farPlane);
}


void CameraModule::rebuildViewProj() {
    ImVec2 sceneSize = m_editorModule->getSceneEditor()->getSize();
    if (sceneSize.x <= 1.0f || sceneSize.y <= 1.0f)
        return;

    unsigned width = static_cast<unsigned>(sceneSize.x);
    unsigned height = static_cast<unsigned>(sceneSize.y);

    Vector3 viewTarget = m_target;

    if (m_editorModule && m_editorModule->getCurrentSceneTool() == EditorModule::SCENE_TOOL::NAVIGATION && m_editorModule->getCurrentNavigationMode() == EditorModule::NAVIGATION_MODE::FREE_LOOK) {
        viewTarget = m_position + getForwardFromYawPitch();
    }

    m_view = Matrix::CreateLookAt(m_position, viewTarget, m_up);
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        m_fovY,
        (height > 0.0f) ? ((float)width / (float)height) : 1.0f,
        m_nearPlane,
        m_farPlane
    );
}

void CameraModule::syncYawPitchFromLookAt() {
    Vector3 dir = m_target - m_position;
    if (dir.LengthSquared() < 1e-8f) {
        m_yaw = 0.0f;
        m_pitch = 0.0f;
        return;
    }
    dir.Normalize();
    m_yaw = atan2f(dir.z, dir.x);
    m_pitch = asinf(dir.y);
}

void CameraModule::syncDistanceFromLookAt() {
    m_distanceToTarget = (m_target - m_position).Length();
    m_distanceToTarget = std::clamp(m_distanceToTarget, m_settings->camera.minDistance, m_settings->camera.maxDistance);
}

Vector3 CameraModule::getForwardFromYawPitch() const {
    Vector3 fwd;
    fwd.x = cosf(m_pitch) * cosf(m_yaw);
    fwd.y = sinf(m_pitch);
    fwd.z = cosf(m_pitch) * sinf(m_yaw);
    fwd.Normalize();
    return fwd;
}

Vector3 CameraModule::getRightFromForward(const Vector3& fwd) const {
    Vector3 right = fwd.Cross(Vector3::Up);
    if (right.LengthSquared() < 1e-8f) right = Vector3::Right;
    right.Normalize();
    return right;
}

void CameraModule::panMode() {
    Mouse::State ms = Mouse::Get().GetState();
    if (!ms.middleButton && !ms.leftButton) {
        m_panFirst = true;
        return;
    }

    ImVec2 sceneMin = m_editorModule->getSceneEditor()->getSize();
    int mx = ms.x - (int)sceneMin.x;
    int my = ms.y - (int)sceneMin.y;

    if (m_panFirst) {
        m_panLastX = mx;
        m_panLastY = my;
        m_panFirst = false;
        return;
    }

    int dx = mx - m_panLastX;
    int dy = my - m_panLastY;
    m_panLastX = mx;
    m_panLastY = my;

    Vector3 forward = m_target - m_position;
    if (forward.LengthSquared() < 1e-8f) return;
    forward.Normalize();

    Vector3 right = forward.Cross(Vector3::Up);
    right.Normalize();
    Vector3 camUp = right.Cross(forward);

    float dist = (m_target - m_position).Length();

    float dxF = (float)dx * (m_settings->camera.panInvertX ? -1.0f : 1.0f);
    float dyF = (float)dy * (m_settings->camera.panInvertY ? -1.0f : 1.0f);

    Vector3 delta = (-right * dxF + camUp * dyF) * (m_settings->camera.panSpeed * dist);

    m_position += delta;
    m_target += delta;
}


void CameraModule::orbitMode() {
    Mouse::State ms = Mouse::Get().GetState();
    ImVec2 sceneMin = m_editorModule->getSceneEditor()->getSize();

    int mx = ms.x - (int)sceneMin.x;
    int my = ms.y - (int)sceneMin.y;

    if (m_orbitFirst) {
        m_orbitLastX = mx;
        m_orbitLastY = my;
        m_orbitFirst = false;
        syncYawPitchFromLookAt();
        syncDistanceFromLookAt();
        return;
    }

    int dx = mx - m_orbitLastX;
    int dy = my - m_orbitLastY;
    m_orbitLastX = mx;
    m_orbitLastY = my;

    float invX = m_settings->camera.orbitInvertX ? -1.0f : 1.0f;
    float invY = m_settings->camera.orbitInvertY ? -1.0f : 1.0f;

    m_yaw += dx * m_settings->camera.orbitSpeed * invX;
    m_pitch += dy * m_settings->camera.orbitSpeed * invY;

    float maxPitch = XM_PIDIV2 - m_settings->camera.flyPitchClamp;
    m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);

    m_position = m_target - getForwardFromYawPitch() * m_distanceToTarget;
}


void CameraModule::zoomMode() {
    Mouse::State ms = Mouse::Get().GetState();
    if (!ms.rightButton) {
        m_zoomFirst = true;
        return;
    }

    ImVec2 sceneMin = m_editorModule->getSceneEditor()->getSize();
    int my = ms.y - (int)sceneMin.y;

    if (m_zoomFirst) {
        m_zoomLastY = my;
        m_zoomFirst = false;
        syncDistanceFromLookAt();
        return;
    }

    int dy = my - m_zoomLastY;
    m_zoomLastY = my;

    m_distanceToTarget = std::clamp(m_distanceToTarget + dy * m_settings->camera.zoomDragSpeed, m_settings->camera.minDistance, m_settings->camera.maxDistance);

    m_position = m_target - getForwardFromYawPitch() * m_distanceToTarget;
}

void CameraModule::handleMouseWheel() {
    Mouse::State ms = Mouse::Get().GetState();

    int delta = ms.scrollWheelValue - m_lastWheel;
    m_lastWheel = ms.scrollWheelValue;

    if (delta == 0) return;

    Vector3 forward = getForwardFromYawPitch();

    if (m_editorModule->getCurrentNavigationMode() == EditorModule::NAVIGATION_MODE::FREE_LOOK) {
        float d = delta * m_settings->camera.zoomWheelSpeed * m_settings->camera.flyMoveSpeed;
        m_position += forward * d;

        m_distanceToTarget = std::clamp(m_distanceToTarget - d, m_settings->camera.minDistance, m_settings->camera.maxDistance);
        return;
    }

    m_distanceToTarget -= delta * m_settings->camera.zoomWheelSpeed;
    m_distanceToTarget = std::clamp(m_distanceToTarget, m_settings->camera.minDistance, m_settings->camera.maxDistance);

    m_position = m_target - forward * m_distanceToTarget;
}

void CameraModule::flythroughMode(float dt) {
    Mouse::State ms = Mouse::Get().GetState();
    Keyboard::State ks = Keyboard::Get().GetState();

    ImVec2 sceneMin = m_editorModule->getSceneEditor()->getSize();
    int mx = ms.x - (int)sceneMin.x;
    int my = ms.y - (int)sceneMin.y;

    if (m_flyFirst) {
        m_flyLastX = mx;
        m_flyLastY = my;
        m_flyFirst = false;
        return;
    }

    int dx = mx - m_flyLastX;
    int dy = my - m_flyLastY;
    m_flyLastX = mx;
    m_flyLastY = my;

    float invX = m_settings->camera.flyInvertX ? -1.0f : 1.0f;
    float invY = m_settings->camera.flyInvertY ? -1.0f : 1.0f;

    m_yaw += dx * m_settings->camera.flyRotSpeed * invX;
    m_pitch += dy * m_settings->camera.flyRotSpeed * invY;

    float maxPitch = XM_PIDIV2 - m_settings->camera.flyPitchClamp;
    m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);

    Vector3 forward = getForwardFromYawPitch();
    Vector3 right = forward.Cross(Vector3::Up);
    right.Normalize();

    float speed = m_settings->camera.flyMoveSpeed * dt;
    if (ks.LeftShift) speed *= m_settings->camera.flyBoostMult;

    if (ks.IsKeyDown(Keyboard::Keys::W)) m_position += forward * speed;
    if (ks.IsKeyDown(Keyboard::Keys::S)) m_position -= forward * speed;
    if (ks.IsKeyDown(Keyboard::Keys::A)) m_position -= right * speed;
    if (ks.IsKeyDown(Keyboard::Keys::D)) m_position += right * speed;
    if (ks.IsKeyDown(Keyboard::Keys::Q)) m_position -= m_up * speed;
    if (ks.IsKeyDown(Keyboard::Keys::E)) m_position += m_up * speed;
}

void CameraModule::handleAutoFocus() {
    Keyboard::State ks = Keyboard::Get().GetState();
    if (ks.IsKeyDown(Keyboard::Keys::F)) {
        focusOnTarget();
    }
}

void CameraModule::focusOnTarget() {
    Vector3 focusPoint = Vector3::Zero;

    GameObject* selectedModel = m_editorModule->getSelectedGameObject();

    if (selectedModel) {
        focusPoint = selectedModel->GetTransform()->getPosition();
    }

    m_distanceToTarget = 10.0f;

    Vector3 forward = getForwardFromYawPitch();

    m_target = focusPoint;
    m_position = m_target - forward * m_distanceToTarget;
}
