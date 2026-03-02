#include "Globals.h"
#include "CameraFollow.h"
#include "Application.h"
#include "SceneModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "TimeModule.h"

static const float PI = 3.1415926535897931f;

CameraFollow::CameraFollow(UID id, GameObject* gameObject)
    : Component(id, ComponentType::CAMERA_FOLLOW, gameObject)
{
}

void CameraFollow::setFollowTargets() 
{
    if (m_firstTargetUid == 0 && m_secondTargetUid != 0)
    {
        m_firstTargetUid = m_secondTargetUid;
        m_secondTargetUid = 0;
        m_firstTargetTransform = nullptr;
        m_secondTargetTransform = nullptr;
    }

    if (m_firstTargetUid == 0)
    {
        m_firstTargetTransform = nullptr;
    }

    if (m_secondTargetUid == 0)
    {
        m_secondTargetTransform = nullptr;
    }

    if (m_firstTargetTransform && (m_secondTargetUid == 0 || m_secondTargetTransform))
    {
        return;
    }

    if (!m_firstTargetTransform && m_firstTargetUid != 0)
    {
        if (GameObject* gameObject = app->getSceneModule()->findGameObjectByUID(m_firstTargetUid)) {
            m_firstTargetTransform = gameObject->GetTransform();
        }
    }

    if (!m_secondTargetTransform && m_secondTargetUid != 0)
    {
        if (GameObject* gameObject = app->getSceneModule()->findGameObjectByUID(m_secondTargetUid)) {
            m_secondTargetTransform = gameObject->GetTransform();
        }
    }
}

void CameraFollow::update()
{
    //This will end up going in the init
    setFollowTargets();
    //
    if (!m_firstTargetTransform) return;

    Transform* cameraTransform = m_owner->GetTransform();
    const float dt = app->getTimeModule()->deltaTime();

    const bool hasSecondTarget = (m_secondTargetTransform != nullptr);

    Vector3 followPoint = computeFollowPoint();

    float targetExtraHeight = 0.0f;
    if (hasSecondTarget)
    {
        const Vector3 p1 = m_firstTargetTransform->getPosition();
        const Vector3 p2 = m_secondTargetTransform->getPosition();
        targetExtraHeight = computeTargetExtraHeight(p1, p2);
    }

    m_currentExtraHeight = smoothExtraHeight(m_currentExtraHeight, targetExtraHeight, m_zoomSharpness, dt);

    const Vector3 desiredPos = computeDesiredCameraPosition(followPoint);

    const Vector3 smoothedCameraPosition = smoothCameraPosition(cameraTransform->getPosition(), desiredPos, m_followSharpness, dt);
    cameraTransform->setPosition(smoothedCameraPosition);

    cameraTransform->setRotationEuler(m_rotationOffset);
}

Vector3 CameraFollow::computeFollowPoint() const
{
    Vector3 followPoint;
    if (!m_secondTargetTransform) {
        followPoint = m_firstTargetTransform->getPosition();

        return followPoint;
    }

    const Vector3 p1 = m_firstTargetTransform->getPosition();
    const Vector3 p2 = m_secondTargetTransform->getPosition();
    followPoint = (p1 + p2) * 0.5f;

    return followPoint;
}

float CameraFollow::computeTargetExtraHeight(const Vector3& p1, const Vector3& p2) const {
    const float distance = (p2 - p1).Length();

    const float zoomRange = m_zoomEndDistance - m_zoomStartDistance;
    const float distancePastZoomStart = distance - m_zoomStartDistance;

    float normalizedZoomFactor = 0.0f;

    if (zoomRange > 0.0001f)
    {
        normalizedZoomFactor = distancePastZoomStart / zoomRange;
        if (normalizedZoomFactor < 0.0f)
        {
            normalizedZoomFactor = 0.0f;
        }
        if (normalizedZoomFactor > 1.0f)
        {
            normalizedZoomFactor = 1.0f;
        }
    }

    float targetExtraHeight = m_maxExtraHeight * normalizedZoomFactor;

    return targetExtraHeight;
}

float CameraFollow::smoothExtraHeight(float currentExtraHeight, float targetExtraHeight, float sharpness, float dt) const {
    if (sharpness <= 0.0f)
    {
        return targetExtraHeight;
    }

    const float zoomFraction = 1.0f - expf(-sharpness * dt);
    return lerpFloat(currentExtraHeight, targetExtraHeight, zoomFraction);
}

Vector3 CameraFollow::computeDesiredCameraPosition(const Vector3& followPoint) const
{
    Vector3 desiredPos = followPoint;

    // XZ follows midpoint
    desiredPos.x += m_transformOffset.x;
    desiredPos.z += m_transformOffset.z;

    // Y follows the highest target
    float highestTargetY = m_firstTargetTransform->getPosition().y;

    if (m_secondTargetTransform)
    {
        const float secondTargetY = m_secondTargetTransform->getPosition().y;
        if (secondTargetY > highestTargetY)
        {
            highestTargetY = secondTargetY;
        }
    }

    desiredPos.y = highestTargetY + m_transformOffset.y + m_currentExtraHeight;

    return desiredPos;
}

Vector3 CameraFollow::smoothCameraPosition(const Vector3& currentPos, const Vector3& targetPos, float sharpness, float dt) const {
    if (sharpness <= 0.0f)
    {
        return targetPos;
    }

    const float followFraction = 1.0f - expf(-sharpness * dt);
    return lerpVector(currentPos, targetPos, followFraction);
}

Vector3 CameraFollow::lerpVector(const Vector3& start, const Vector3& end, float alpha) const
{
    return start + (end - start) * alpha;
}

float CameraFollow::lerpFloat(float start, float end, float alpha) const
{
    return start + (end - start) * alpha;
}

void CameraFollow::drawUi()
{
    ImGui::Text("Camera Follow");
    ImGui::SeparatorText("First Target");

    if (ImGui::InputScalar("First UID", ImGuiDataType_U64, &m_firstTargetUid))
    {
        m_firstTargetTransform = nullptr;
    }
    ImGui::Text("First Target Set: %s", (m_firstTargetTransform ? "YES" : "NO"));

    ImGui::SeparatorText("Second Target");

    if (ImGui::InputScalar("Second UID", ImGuiDataType_U64, &m_secondTargetUid))
    {
        m_secondTargetTransform = nullptr;
    }
    ImGui::Text("Second Target Set: %s", (m_secondTargetTransform ? "YES" : "NO"));

    ImGui::SeparatorText("Camera Transform");

    ImGui::DragFloat3("World Offset", &m_transformOffset.x, 0.05f);
    ImGui::DragFloat3("Fixed Rotation", &m_rotationOffset.x, 0.25f);

    ImGui::SeparatorText("Smoothing");

    ImGui::DragFloat("Follow Sharpness", &m_followSharpness, 0.1f, 0.0f, 50.0f);
    ImGui::DragFloat("Zoom Sharpness", &m_zoomSharpness, 0.1f, 0.0f, 50.0f);

    ImGui::SeparatorText("Zoom by Distance (2 targets)");

    ImGui::DragFloat("Zoom Start Distance", &m_zoomStartDistance, 0.05f, 0.0f, 1000.0f);
    ImGui::DragFloat("Zoom End Distance", &m_zoomEndDistance, 0.05f, 0.0f, 1000.0f);
    ImGui::DragFloat("Max Extra Height", &m_maxExtraHeight, 0.05f, 0.0f, 1000.0f);
}