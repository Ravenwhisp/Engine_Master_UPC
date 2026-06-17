#include "pch.h"
#include "TargetIndicatorUI.h"
#include "PlayerTargetController.h"

IMPLEMENT_SCRIPT_FIELDS(TargetIndicatorUI,
    SERIALIZED_COMPONENT_REF(m_playerTransform, "Player Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_targetIndicatorTransform, "Target Indicator Transform", ComponentType::TRANSFORM),
    SERIALIZED_VEC3(m_positionOffset, "Position Offset"),
    SERIALIZED_FLOAT(m_followSharpness, "Follow Sharpness", 0.0f, 50.0f, 0.1f),
    FIELD_GROUP_LABEL("Animation"),
    SERIALIZED_FLOAT(m_switchPopScale, "Switch Pop Scale", 1.0f, 3.0f, 0.05f),
    SERIALIZED_FLOAT(m_switchPopDuration, "Switch Pop Duration", 0.01f, 1.0f, 0.01f)
)

TargetIndicatorUI::TargetIndicatorUI(GameObject* owner)
    : Script(owner)
{
}

void TargetIndicatorUI::Start()
{
    m_playerTargetController = getPlayerTargetController();

    Transform* visualTransform = m_targetIndicatorTransform.getReferencedComponent();
    if (visualTransform != nullptr)
    {
        m_targetIndicatorBaseScale = TransformAPI::getScale(visualTransform);
    }

    onStart();

    hideTargetIndicator();
    hideDirectionIndicator();
}

void TargetIndicatorUI::Update()
{
    if (m_playerTargetController == nullptr)
    {
        m_playerTargetController = getPlayerTargetController();
    }

    if (m_playerTargetController == nullptr)
    {
        hideTargetIndicator();
        hideDirectionIndicator();
        return;
    }

    GameObject* currentTarget = m_playerTargetController->getCurrentTarget();
    if (currentTarget == nullptr)
    {
        hideTargetIndicator();
        hideDirectionIndicator();
        m_previousTarget = nullptr;
        return;
    }

    updateTargetIndicator(currentTarget);
    updateDirectionIndicator(currentTarget);
}

void TargetIndicatorUI::updateTargetIndicator(GameObject* currentTarget)
{
    Transform* visualTransform = m_targetIndicatorTransform.getReferencedComponent();
    if (visualTransform == nullptr)
    {
        hideTargetIndicator();
        return;
    }

    if (currentTarget == nullptr)
    {
        hideTargetIndicator();
        return;
    }

    Transform* targetTransform = GameObjectAPI::getTransform(currentTarget);
    if (targetTransform == nullptr)
    {
        hideTargetIndicator();
        return;
    }

    showTargetIndicator();

    const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);
    const Vector3 desiredPosition = targetPosition + m_positionOffset;

    const bool targetChanged = (currentTarget != m_previousTarget);

    if (targetChanged)
    {
        TransformAPI::setGlobalPosition(visualTransform, desiredPosition);
        startSwitchAnimation();
        m_previousTarget = currentTarget;
    }
    else if (m_followSharpness <= 0.0f)
    {
        TransformAPI::setGlobalPosition(visualTransform, desiredPosition);
    }
    else
    {
        const Vector3 currentPosition = TransformAPI::getGlobalPosition(visualTransform);
        const float dt = Time::getDeltaTime();
        const float followFraction = 1.0f - expf(-m_followSharpness * dt);
        const Vector3 smoothedPosition = currentPosition + (desiredPosition - currentPosition) * followFraction;

        TransformAPI::setGlobalPosition(visualTransform, smoothedPosition);
    }

    updateSwitchAnimation(visualTransform);
}

PlayerTargetController* TargetIndicatorUI::getPlayerTargetController() const
{
    Transform* playerTransform = m_playerTransform.getReferencedComponent();
    if (playerTransform == nullptr)
    {
        return nullptr;
    }

    GameObject* player = ComponentAPI::getOwner(playerTransform);

    return GameObjectAPI::findScript<PlayerTargetController>(player);
}

void TargetIndicatorUI::hideTargetIndicator()
{
    Transform* visualTransform = m_targetIndicatorTransform.getReferencedComponent();
    if (visualTransform == nullptr)
    {
        return;
    }

    m_switchAnimationTimer = 0.0f;
    TransformAPI::setScale(visualTransform, m_targetIndicatorBaseScale);

    setVisualActive(visualTransform, false);
}

void TargetIndicatorUI::showTargetIndicator()
{
    Transform* visualTransform = m_targetIndicatorTransform.getReferencedComponent();
    if (visualTransform == nullptr)
    {
        return;
    }

    setVisualActive(visualTransform, true);
}

void TargetIndicatorUI::startSwitchAnimation()
{
    m_switchAnimationTimer = m_switchPopDuration;
}

void TargetIndicatorUI::updateSwitchAnimation(Transform* targetIndicatorTransform)
{
    if (targetIndicatorTransform == nullptr)
    {
        return;
    }

    if (m_switchAnimationTimer <= 0.0f || m_switchPopDuration <= 0.0001f)
    {
        TransformAPI::setScale(targetIndicatorTransform, m_targetIndicatorBaseScale);
        return;
    }

    m_switchAnimationTimer -= Time::getDeltaTime();

    if (m_switchAnimationTimer < 0.0f)
    {
        m_switchAnimationTimer = 0.0f;
    }

    const float normalizedTime = 1.0f - (m_switchAnimationTimer / m_switchPopDuration);

    const float popAmount = sinf(normalizedTime * 3.14159265f);
    const float scaleMultiplier = 1.0f + ((m_switchPopScale - 1.0f) * popAmount);

    TransformAPI::setScale(targetIndicatorTransform, m_targetIndicatorBaseScale * scaleMultiplier);
}

bool TargetIndicatorUI::tryGetFlatDirectionToTarget(GameObject* currentTarget, Vector3& outPlayerPosition, Vector3& outDirection, float* outDistance) const
{
    if (outDistance != nullptr)
    {
        *outDistance = 0.0f;
    }

    if (currentTarget == nullptr)
    {
        return false;
    }

    Transform* playerTransform = m_playerTransform.getReferencedComponent();
    Transform* targetTransform = GameObjectAPI::getTransform(currentTarget);

    if (playerTransform == nullptr || targetTransform == nullptr)
    {
        return false;
    }

    outPlayerPosition = TransformAPI::getGlobalPosition(playerTransform);
    const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 flatPlayerPosition = outPlayerPosition;
    Vector3 flatTargetPosition = targetPosition;

    flatPlayerPosition.y = 0.0f;
    flatTargetPosition.y = 0.0f;

    outDirection = flatTargetPosition - flatPlayerPosition;
    outDirection.y = 0.0f;

    const float distanceSq = outDirection.LengthSquared();
    if (distanceSq <= 0.0001f)
    {
        return false;
    }

    if (outDistance != nullptr)
    {
        *outDistance = sqrtf(distanceSq);
    }

    outDirection.Normalize();
    return true;
}

void TargetIndicatorUI::setVisualActive(Transform* visualTransform, bool active) const
{
    if (visualTransform == nullptr)
    {
        return;
    }

    GameObject* visualObject = ComponentAPI::getOwner(visualTransform);
    if (visualObject == nullptr)
    {
        return;
    }

    GameObjectAPI::setActive(visualObject, active);
}

float TargetIndicatorUI::getDirectionAngleDegrees(const Vector3& direction, float rotationOffsetDegrees) const
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() <= 0.0001f)
    {
        return rotationOffsetDegrees;
    }

    flatDirection.Normalize();

    const float angleRadians = atan2f(flatDirection.z, flatDirection.x);
    return DirectX::XMConvertToDegrees(angleRadians) + rotationOffsetDegrees;
}

IMPLEMENT_SCRIPT(TargetIndicatorUI)