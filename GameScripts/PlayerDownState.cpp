#include "pch.h"
#include "PlayerDownState.h"
#include "Damageable.h"
#include "PlayerState.h"

IMPLEMENT_SCRIPT_FIELDS(PlayerDownState,
    SERIALIZED_FLOAT(m_selfReviveTime, "Self Revive Time", 0.1f, 999999.0f, 0.1f),
    SERIALIZED_FLOAT(m_assistRadius, "Assist Radius", 0.0f, 999999.0f, 0.1f),
    SERIALIZED_FLOAT(m_assistSpeedMultiplier, "Assist Speed Multiplier", 1.0f, 999999.0f, 0.1f),
    SERIALIZED_FLOAT(m_reviveHp, "Revive HP", 1.0f, 999999.0f, 1.0f),
    SERIALIZED_COMPONENT_REF(m_teammateTransform, "Teammate Transform", ComponentType::TRANSFORM)
)

PlayerDownState::PlayerDownState(GameObject* owner)
    : Script(owner)
{
}

void PlayerDownState::Start()
{
    m_damageable = findDamageable();

    if (!m_damageable)
    {
        Debug::warn("PlayerDownState on '%s' could not find a Damageable on the same GameObject.", GameObjectAPI::getName(m_owner));
    }

    Script* stateScript = GameObjectAPI::getScript(m_owner, "PlayerState");
    m_playerState = dynamic_cast<PlayerState*>(stateScript);

    if (!m_playerState)
    {
        Debug::warn("PlayerDownState on '%s' could not find PlayerState on the same GameObject.", GameObjectAPI::getName(m_owner));
    }
}

void PlayerDownState::Update()
{
    if (!isDowned())
    {
        return;
    }

    if (!m_damageable)
    {
        return;
    }

    float speedMultiplier = 1.0f;
    if (isTeammateInAssistRange()) 
    {
        speedMultiplier *= m_assistSpeedMultiplier;
    }

    m_reviveProgress += Time::getDeltaTime() * speedMultiplier;

    if (m_reviveProgress >= m_selfReviveTime)
    {
        completeRevive();
    }
}

void PlayerDownState::drawGizmo()
{
    Transform* transform = GameObjectAPI::getTransform(m_owner);
    Vector3 position = TransformAPI::getPosition(transform);

    Vector3 circleColor = Vector3(0.0f, 1.0f, 0.0f);
    if (isDowned())
    {
        circleColor = Vector3(1.0f, 1.0f, 0.0f);
    }

    DebugDrawAPI::drawCircle(position, Vector3(0.0f, 1.0f, 0.0f), circleColor, m_assistRadius, 32.0f, 0, true);
}

void PlayerDownState::enterDownState()
{
    if (isDowned())
    {
        return;
    }

    if (m_playerState)
    {
        m_playerState->setState(PlayerStateType::Downed);
    }

    m_reviveProgress = 0.0f;

    if (m_damageable)
    {
        m_damageable->setInvulnerable(true);
    }

    Debug::log("%s entered down state.", GameObjectAPI::getName(m_owner));
}

bool PlayerDownState::isDowned() const
{
    return m_playerState && m_playerState->isDowned();
}

float PlayerDownState::getReviveProgress() const
{
    if (m_selfReviveTime <= 0.0f)
    {
        return 1.0f;
    }

    float progress = m_reviveProgress/m_selfReviveTime;

    if (progress < 0.0f)
    {
        return 0.0f;
    }

    if (progress > 1.0f)
    {
        return 1.0f;
    }

    return progress;
}

Damageable* PlayerDownState::findDamageable() const
{
    Script* script = GameObjectAPI::getScript(m_owner, "PlayerDamageable");
    Damageable* damageable = dynamic_cast<Damageable*>(script);

    if (damageable)
    {
        return damageable;
    }

    script = GameObjectAPI::getScript(m_owner, "Damageable");
    damageable = dynamic_cast<Damageable*>(script);

    return damageable;
}

bool PlayerDownState::isTeammateInAssistRange() const
{
    Transform* ownTransform = GameObjectAPI::getTransform(m_owner);
    Transform* teammateTransform = m_teammateTransform.getReferencedComponent();

    if (!ownTransform || !teammateTransform)
    {
        return false;
    }

    Vector3 ownPosition = TransformAPI::getPosition(ownTransform);
    Vector3 teammatePosition = TransformAPI::getPosition(teammateTransform);

    float distance = (teammatePosition - ownPosition).Length();

    return distance <= m_assistRadius;
}

void PlayerDownState::completeRevive()
{
    m_reviveProgress = 0.0f;

    if (m_playerState)
    {
        m_playerState->setState(PlayerStateType::Normal);
    }

    if (m_damageable)
    {
        m_damageable->setInvulnerable(false);
        m_damageable->revive(m_reviveHp);
    }

    Debug::log("%s completed revive from down state.", GameObjectAPI::getName(m_owner));
}

IMPLEMENT_SCRIPT(PlayerDownState)