#include "pch.h"
#include "PlayerDamageable.h"
#include "HeartbeatHaptic.h"

#include "PlayerDownState.h"
#include "PlayerAnimationController.h"

IMPLEMENT_SCRIPT_FIELDS(PlayerDamageable,
    SERIALIZED_FLOAT(m_heartbeatThreshold, "Heartbeat Threshold", 0.5f, 0.25f, 0.0f)
)

PlayerDamageable::PlayerDamageable(GameObject* owner)
    : Damageable(owner)
{
}

void PlayerDamageable::Start()
{
    Damageable::Start();

    m_playerAnimationController = GameObjectAPI::findScript<PlayerAnimationController>(m_owner);
    if (m_playerAnimationController == nullptr)
        Debug::warn("%s has PlayerDamageable but no PlayerAnimationController.", GameObjectAPI::getName(m_owner));

    m_haptic = GameObjectAPI::findScript<HeartbeatHaptic>(m_owner);
    if (m_haptic != nullptr)
        m_haptic->m_variant = HapticEffectDefinition::HeartbeatVariant::Health;
}

void PlayerDamageable::Update()
{
    if (!m_haptic) return;

    if (isDead())
        return; // dying beat is self-terminating; don't interfere

    if (getHpPercent() >= m_heartbeatThreshold)
    {
        m_haptic->stop();
        return;
    }

    // danger is the inverse of HP, drives beat speed and intensity
    const float danger = 1.0f - getHpPercent();
    m_haptic->tick(danger);
}

void PlayerDamageable::onDamaged(float amount)
{
    Damageable::onDamaged(amount);

    if (m_playerAnimationController != nullptr)
        m_playerAnimationController->requestDamaged();
}

void PlayerDamageable::onHpDepleted()
{
    PlayerDownState* downState = GameObjectAPI::findScript<PlayerDownState>(m_owner);

    if (downState)
    {
        downState->enterDownState();

        if (m_playerAnimationController != nullptr)
            m_playerAnimationController->setDowned(true);

        return;
    }

    Debug::warn("%s has PlayerDamageable but no PlayerDownState. Falling back to kill.", GameObjectAPI::getName(m_owner));
    Damageable::onHpDepleted();
}

void PlayerDamageable::onDeath()
{
    Damageable::onDeath();

    if (m_playerAnimationController != nullptr)
        m_playerAnimationController->setDead(true);

    if (m_haptic)
    {
        const float danger = 1.0f - getHpPercent();
        m_haptic->playDyingBeat(danger);
    }
}

void PlayerDamageable::onRevive()
{
    Damageable::onRevive();

    if (m_playerAnimationController != nullptr)
    {
        m_playerAnimationController->setDead(false);
        m_playerAnimationController->setDowned(false);
    }

    if (m_haptic) m_haptic->stop();
}

IMPLEMENT_SCRIPT(PlayerDamageable)