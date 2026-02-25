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

    Vector3 followPoint = m_firstTargetTransform->getPosition();

    const bool hasSecondTarget = (m_secondTargetTransform != nullptr);
    if (hasSecondTarget)
    {
        const Vector3 p1 = m_firstTargetTransform->getPosition();
        const Vector3 p2 = m_secondTargetTransform->getPosition();
        followPoint = (p1 + p2) * 0.5f; 
    }

    const Vector3 desiredPos = followPoint + m_transformOffset;

    if (m_followSharpness <= 0.0f)
    {
        cameraTransform->setPosition(desiredPos);
    }
    else
    {
        const float followFraction = 1.0f - expf(-m_followSharpness * dt);
        cameraTransform->setPosition(lerpVector(cameraTransform->getPosition(), desiredPos, followFraction));
    }

    cameraTransform->setRotationEuler(m_rotationOffset);
}

Vector3 CameraFollow::lerpVector(const Vector3& start, const Vector3& end, float alpha) const
{
    const Vector3 directionFromStartToEnd = end - start;
    const Vector3 stepTowardEnd = directionFromStartToEnd * alpha;

    return start + stepTowardEnd;
}

void CameraFollow::drawUi()
{
    ImGui::Text("Camera Follow");
    if (ImGui::InputScalar("First Target UID", ImGuiDataType_U64, &m_firstTargetUid))
    {
        m_firstTargetTransform = nullptr;
    }
    ImGui::Text("First Target Set: %s", (m_firstTargetTransform ? "YES" : "NO"));

    if (ImGui::InputScalar("Second Target UID", ImGuiDataType_U64, &m_secondTargetUid))
    {
        m_secondTargetTransform = nullptr;
    }
    ImGui::Text("Second Target Set: %s", (m_secondTargetTransform ? "YES" : "NO"));

    ImGui::Separator();

    ImGui::DragFloat3("World Offset", &m_transformOffset.x, 0.05f);
    ImGui::DragFloat3("Fixed Rotation", &m_rotationOffset.x, 0.25f);
    ImGui::DragFloat("Sharpness", &m_followSharpness, 0.1f, 0.0f, 50.0f);
}