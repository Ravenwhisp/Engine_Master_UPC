#include "pch.h"
#include "TargetIndicatorUI.h"
#include "PlayerTargetController.h"

static const ScriptFieldInfo targetIndicatorUIFields[] =
{
    { "Player Transform", ScriptFieldType::ComponentRef, offsetof(TargetIndicatorUI, m_playerTransform), {}, {}, { ComponentType::TRANSFORM } },
    { "Indicator Visual Transform", ScriptFieldType::ComponentRef, offsetof(TargetIndicatorUI, m_indicatorVisualTransform), {}, {}, { ComponentType::TRANSFORM } },
    { "Position Offset", ScriptFieldType::Vec3, offsetof(TargetIndicatorUI, m_positionOffset) },
    { "Follow Sharpness", ScriptFieldType::Float, offsetof(TargetIndicatorUI, m_followSharpness), { 0.0f, 50.0f, 0.1f } }
};

IMPLEMENT_SCRIPT_FIELDS(TargetIndicatorUI, targetIndicatorUIFields)

TargetIndicatorUI::TargetIndicatorUI(GameObject* owner)
    : Script(owner)
{
}

void TargetIndicatorUI::Start()
{
    m_playerTargetController = getPlayerTargetController();
    hideIndicator();

}

void TargetIndicatorUI::Update()
{
    if (m_playerTargetController == nullptr)
    {
        m_playerTargetController = getPlayerTargetController();
    }

    if (m_playerTargetController == nullptr)
    {
        hideIndicator();
        return;
    }

    Transform* visualTransform = m_indicatorVisualTransform.getReferencedComponent();
    if (visualTransform == nullptr)
    {
        hideIndicator();
        return;
    }

    GameObject* currentTarget = m_playerTargetController->getCurrentTarget();
    if (currentTarget == nullptr)
    {
        hideIndicator();
        return;
    }

    Transform* targetTransform = GameObjectAPI::getTransform(currentTarget);

    showIndicator();

    const Vector3 targetPosition = TransformAPI::getPosition(targetTransform);
    const Vector3 desiredPosition = targetPosition + m_positionOffset;

    const bool targetChanged = (currentTarget != m_previousTarget);

    if (targetChanged || m_followSharpness <= 0.0f)
    {
        TransformAPI::setPosition(visualTransform, desiredPosition);
        return;
    }
    else {
        const Vector3 currentPosition = TransformAPI::getPosition(visualTransform);
        const float dt = Time::getDeltaTime();
        const float followFraction = 1.0f - expf(-m_followSharpness * dt);
        const Vector3 smoothedPosition = currentPosition + (desiredPosition - currentPosition) * followFraction;

        TransformAPI::setPosition(visualTransform, smoothedPosition);
    }

    m_previousTarget = currentTarget;
}

PlayerTargetController* TargetIndicatorUI::getPlayerTargetController() const
{
    Transform* playerTransform = m_playerTransform.getReferencedComponent();
    if (playerTransform == nullptr)
    {
        return nullptr;
    }

    GameObject* player = ComponentAPI::getOwner(playerTransform);

    Script* script = GameObjectAPI::getScript(player, "PlayerTargetController");
    if (script == nullptr)
    {
        return nullptr;
    }

    return static_cast<PlayerTargetController*>(script);
}

void TargetIndicatorUI::hideIndicator()
{
    Transform* visualTransform = m_indicatorVisualTransform.getReferencedComponent();
    if (visualTransform == nullptr)
    {
        return;
    }

    GameObject* visualObject = ComponentAPI::getOwner(visualTransform);
    GameObjectAPI::setActive(visualObject, false);
}

void TargetIndicatorUI::showIndicator()
{
    Transform* visualTransform = m_indicatorVisualTransform.getReferencedComponent();
    if (visualTransform == nullptr)
    {
        return;
    }

    GameObject* visualObject = ComponentAPI::getOwner(visualTransform);
    GameObjectAPI::setActive(visualObject, true);
}

IMPLEMENT_SCRIPT(TargetIndicatorUI)