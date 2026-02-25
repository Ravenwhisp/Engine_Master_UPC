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

void CameraFollow::setFollowTarget() 
{
    if (m_targetUid == 0)
    {
        m_targetTransform = nullptr;
        return;
    }

    if (m_targetTransform) return;

    GameObject* targetGameObject = app->getSceneModule()->findGameObjectByUID(m_targetUid);
    if (!targetGameObject)
    {
        return;
    }

    m_targetTransform = targetGameObject->GetTransform();
}

void CameraFollow::update()
{
    //This will end up going in the init
    setFollowTarget();
    //
    if (!m_targetTransform) return;

    Transform* transform = m_owner->GetTransform();

    const float dt = app->getTimeModule()->deltaTime();
    const Vector3 targetPos = m_targetTransform->getPosition();

    const Vector3 desiredPos = targetPos + m_transformOffset;;

    if (m_followSharpness <= 0.0f)
    {
        transform->setPosition(desiredPos);
    }
    else
    {
        const float followFraction = 1.0f - expf(-m_followSharpness * dt);
        transform->setPosition(lerpVector(transform->getPosition(), desiredPos, followFraction));
    }

    transform->setRotationEuler(m_rotationOffset);
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
    if (ImGui::InputScalar("Target UID", ImGuiDataType_U64, &m_targetUid))
    {
        m_targetTransform = nullptr;
    }
    ImGui::Text("UID Set: %s", (m_targetTransform ? "YES" : "NO"));

    ImGui::Separator();

    ImGui::DragFloat3("world Offset", &m_transformOffset.x, 0.05f);
    ImGui::DragFloat3("Fixed Euler", &m_rotationOffset.x, 0.25f);
    ImGui::DragFloat("Sharpness", &m_followSharpness, 0.1f, 0.0f, 50.0f);
}