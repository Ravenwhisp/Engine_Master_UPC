#include "pch.h"
#include "PlayerDamageable.h"

#include "PlayerDownState.h"
#include "PlayerAnimationController.h"

PlayerDamageable::PlayerDamageable(GameObject* owner)
    : Damageable(owner)
{
}

void PlayerDamageable::Start()
{
    Damageable::Start();

    m_playerAnimationController = GameObjectAPI::findScript<PlayerAnimationController>(m_owner);

    if (m_playerAnimationController == nullptr)
    {
        Debug::warn("%s has PlayerDamageable but no PlayerAnimationController.", GameObjectAPI::getName(m_owner));
    }

    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatLub(1.0f, HapticEffectDefinition::HeartbeatVariant::Health));
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatDub(1.0f, HapticEffectDefinition::HeartbeatVariant::Health));
}

void PlayerDamageable::Update()
{
    const float dt = Time::getDeltaTime();

    if (getHpPercent() >= m_heartbeatThreshold && !m_dyingBeat)
    {
        m_dubTimer = -1.0f;
        m_lubTimer = -1.0f;
        return;
    }

    if (m_dubTimer < 0.0f && m_lubTimer < 0.0f && !isDead())
        fireLub();

    if (m_dubTimer >= 0.0f)
    {
        m_dubTimer -= dt;
        if (m_dubTimer < 0.0f)
        {
            HapticAPI::playAtScale("HeartbeatDub_Health", m_dubScale, 0);

            if (m_dyingBeat)
            {
                m_dyingBeat = false;
                m_dubTimer = -1.0f;
                m_lubTimer = -1.0f;
                return;
            }

            const HeartbeatCycle cycle = HeartbeatCycle::fromHealth(getHpPercent());
            m_lubTimer = cycle.diastoleSeconds;
        }
    }

    if (m_lubTimer >= 0.0f)
    {
        m_lubTimer -= dt;
        if (m_lubTimer < 0.0f)
            fireLub();
    }
}

void PlayerDamageable::fireLub()
{
    const float danger = 1.0f - getHpPercent();
    const HeartbeatCycle cycle = HeartbeatCycle::fromHealth(getHpPercent());

    HapticAPI::playAtScale("HeartbeatLub_Health", danger, 0);

    m_dubScale = danger;
    m_dubTimer = cycle.interBeatSeconds;
    m_lubTimer = -1.0f;
}

void PlayerDamageable::onDamaged(float amount)
{
    Damageable::onDamaged(amount);

    if (m_playerAnimationController != nullptr)
    {
        m_playerAnimationController->requestDamaged();
    }
}

void PlayerDamageable::onHpDepleted()
{
    PlayerDownState* downState = GameObjectAPI::findScript<PlayerDownState>(m_owner);

    if (downState)
    {
        downState->enterDownState();

        if (m_playerAnimationController != nullptr)
        {
            m_playerAnimationController->setDowned(true);
        }

        return;
    }

    Debug::warn("%s has PlayerDamageable but no PlayerDownState. Falling back to kill.", GameObjectAPI::getName(m_owner));
    Damageable::onHpDepleted();
}

void PlayerDamageable::onDeath()
{
    Damageable::onDeath();

    if (m_playerAnimationController != nullptr)
    {
        m_playerAnimationController->setDead(true);
    }

    m_dyingBeat = true;
    fireLub();
}

void PlayerDamageable::onRevive()
{
    Damageable::onRevive();

    if (m_playerAnimationController != nullptr)
    {
        m_playerAnimationController->setDead(false);
        m_playerAnimationController->setDowned(false);
    }

    m_dubTimer = -1.0f;
    m_lubTimer = -1.0f;
    m_dubScale = 0.0f;
    m_dyingBeat = false;
}

IMPLEMENT_SCRIPT(PlayerDamageable)