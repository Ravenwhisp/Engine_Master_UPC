#include "pch.h"
#include "CameraFollow.h"
#include "ScriptAPI.h"

static const float PI = 3.1415926535897931f;

static const ScriptFieldInfo cameraFollowFields[] =
{
    { "First Target", ScriptFieldType::ComponentRef, offsetof(CameraFollow, m_firstTarget), {}, {},{ ComponentType::TRANSFORM } },
    { "Second Target", ScriptFieldType::ComponentRef, offsetof(CameraFollow, m_secondTarget), {}, {}, { ComponentType::TRANSFORM } },
    { "World Offset", ScriptFieldType::Vec3, offsetof(CameraFollow, m_transformOffset) },
    { "Fixed Rotation", ScriptFieldType::Vec3, offsetof(CameraFollow, m_rotationOffset) },
    { "Follow Sharpness", ScriptFieldType::Float, offsetof(CameraFollow, m_followSharpness), { 0.0f, 50.0f, 0.1f } },
    { "Zoom Sharpness", ScriptFieldType::Float, offsetof(CameraFollow, m_zoomSharpness), { 0.0f, 50.0f, 0.1f } },
    { "Zoom Start Distance", ScriptFieldType::Float, offsetof(CameraFollow, m_zoomStartDistance), { 0.0f, 1000.0f, 0.05f } },
    { "Zoom End Distance", ScriptFieldType::Float, offsetof(CameraFollow, m_zoomEndDistance), { 0.0f, 1000.0f, 0.05f } },
    { "Max Extra Height", ScriptFieldType::Float, offsetof(CameraFollow, m_maxExtraHeight), { 0.0f, 1000.0f, 0.05f } }
};

IMPLEMENT_SCRIPT_FIELDS(CameraFollow, cameraFollowFields)

CameraFollow::CameraFollow(GameObject* owner)
    : Script(owner)
{
}

void CameraFollow::Start()
{
}

void CameraFollow::Update()
{
    Transform* firstTarget = m_firstTarget.get();
    if (!firstTarget)
    {
        return;
    }

    GameObject* camera = getOwner();
    Transform* cameraTransform = GameObjectAPI::getTransform(camera);

    Transform* secondTarget = m_secondTarget.get();
    const bool hasSecondTarget = (secondTarget != nullptr);

    const float dt = Time::getDeltaTime();

    Vector3 followPoint = computeFollowPoint();

    float targetExtraHeight = 0.0f;
    if (hasSecondTarget)
    {
        const Vector3 p1 = TransformAPI::getPosition(firstTarget);
        const Vector3 p2 = TransformAPI::getPosition(secondTarget);
        targetExtraHeight = computeTargetExtraHeight(p1, p2);
    }

    m_currentExtraHeight = smoothExtraHeight(m_currentExtraHeight, targetExtraHeight, m_zoomSharpness, dt);

    const Vector3 desiredPos = computeDesiredCameraPosition(followPoint);

    if (m_firstUpdateAfterResolve)
    {
        TransformAPI::setPosition(cameraTransform, desiredPos);
        TransformAPI::setRotationEuler(cameraTransform, m_rotationOffset);
        m_firstUpdateAfterResolve = false;
        return;
    }

    const Vector3 currentPos = TransformAPI::getPosition(cameraTransform);
    const Vector3 smoothedCameraPosition = smoothCameraPosition(currentPos, desiredPos, m_followSharpness, dt);

    TransformAPI::setPosition(cameraTransform, smoothedCameraPosition);
    TransformAPI::setRotationEuler(cameraTransform, m_rotationOffset);
}

void CameraFollow::onAfterReferencesFixed()
{
    m_currentExtraHeight = 0.0f;
    m_firstUpdateAfterResolve = true;

    if (!m_firstTarget.get() && m_secondTarget.get())
    {
        m_firstTarget = m_secondTarget;
        m_secondTarget.clear();
    }
}

Vector3 CameraFollow::computeFollowPoint() const
{
    Transform* firstTarget = m_firstTarget.get();
    Transform* secondTarget = m_secondTarget.get();
    if (!secondTarget)
    {
        return TransformAPI::getPosition(firstTarget);
    }

    GameObject* secondTargetOwner = ComponentAPI::getOwner(secondTarget);

    const Vector3 p1 = TransformAPI::getPosition(firstTarget);
    const Vector3 p2 = TransformAPI::getPosition(secondTarget);
    return (p1 + p2) * 0.5f;
}

float CameraFollow::computeTargetExtraHeight(const Vector3& p1, const Vector3& p2) const
{
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

    return m_maxExtraHeight * normalizedZoomFactor;
}

float CameraFollow::smoothExtraHeight(float current, float target, float sharpness, float dt) const
{
    if (sharpness <= 0.0f)
    {
        return target;
    }

    const float zoomFraction = 1.0f - expf(-sharpness * dt);
    return lerpFloat(current, target, zoomFraction);
}

Vector3 CameraFollow::computeDesiredCameraPosition(const Vector3& followPoint) const
{
    Transform* firstTarget = m_firstTarget.get();
    Transform* secondTarget = m_secondTarget.get();

    Vector3 desiredPos = followPoint;

    desiredPos.x += m_transformOffset.x;
    desiredPos.z += m_transformOffset.z;

    float highestTargetY = TransformAPI::getPosition(firstTarget).y;

    if (secondTarget)
    {
        const float secondTargetY = TransformAPI::getPosition(secondTarget).y;
        if (secondTargetY > highestTargetY)
        {
            highestTargetY = secondTargetY;
        }
    }

    desiredPos.y = highestTargetY + m_transformOffset.y + m_currentExtraHeight;

    return desiredPos;
}

Vector3 CameraFollow::smoothCameraPosition(const Vector3& current, const Vector3& target, float sharpness, float dt) const
{
    if (sharpness <= 0.0f)
    {
        return target;
    }

    const float followFraction = 1.0f - expf(-sharpness * dt);
    return lerpVector(current, target, followFraction);
}

Vector3 CameraFollow::lerpVector(const Vector3& start, const Vector3& end, float alpha) const
{
    return start + (end - start) * alpha;
}

float CameraFollow::lerpFloat(float start, float end, float alpha) const
{
    return start + (end - start) * alpha;
}

IMPLEMENT_SCRIPT(CameraFollow)