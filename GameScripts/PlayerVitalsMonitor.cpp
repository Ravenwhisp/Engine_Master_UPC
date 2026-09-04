#include "pch.h"
#include "PlayerVitalsMonitor.h"
#include "PlayerDamageable.h"
#include "Bound.h"

IMPLEMENT_SCRIPT_FIELDS(PlayerVitalsMonitor,
    SERIALIZED_BOOL(m_autoEnableHeartbeat, "Auto Enable Heartbeat"),
    SERIALIZED_FLOAT(m_healthThreshold, "Health Threshold", 0.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_heartbeatIntensityScale, "Heartbeat Intensity Scale", 0.1f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_deathGreyDuration, "Death Grey Duration", 0.0f, 60.0f, 0.1f),
    SERIALIZED_FLOAT(m_deathBlackDuration, "Death Black Duration", 0.0f, 60.0f, 0.1f)
)

PlayerVitalsMonitor::PlayerVitalsMonitor(GameObject* owner)
    : Script(owner)
{
}

void PlayerVitalsMonitor::Start()
{
    findPlayers();
    findBound();

    if (!m_player1 || !m_player2)
    {
        Debug::warn("PlayerVitalsMonitor on '%s' could not find both players at Start; will keep looking each frame.", GameObjectAPI::getName(m_owner));
    }

    if (!m_bound)
    {
        Debug::warn("PlayerVitalsMonitor on '%s' could not find a Bound in the scene; will keep looking each frame.", GameObjectAPI::getName(m_owner));
    }

    // Reset post-process state so a fresh run/scene restart never inherits a
    // stale death fade or heartbeat from a previous session.
    PostProcessAPI::setHeartbeatEnabled(m_autoEnableHeartbeat);
    PostProcessAPI::setHealthThreshold(kEngineThresholdPassthrough);
    PostProcessAPI::setDeathGreyDuration(m_deathGreyDuration);
    PostProcessAPI::setDeathBlackDuration(m_deathBlackDuration);

    PostProcessAPI::setHealth(1.0f);
    PostProcessAPI::setSeparation(0.0f);
    PostProcessAPI::setDeathFadeActive(false);
}

void PlayerVitalsMonitor::Update()
{
    if (!allPlayersFound())
    {
        findPlayers();
    }

    if (!m_bound)
    {
        findBound();
    }

    float lowestHpPercent = 1.0f;
    bool anyPlayerFound = false;

    if (m_player1 != nullptr)
    {
        lowestHpPercent = min(lowestHpPercent, m_player1->getHpPercent());
        anyPlayerFound = true;
    }

    if (m_player2 != nullptr)
    {
        lowestHpPercent = min(lowestHpPercent, m_player2->getHpPercent());
        anyPlayerFound = true;
    }

    float visualHealth = 1.0f;

    if (anyPlayerFound && lowestHpPercent < m_healthThreshold)
    {
        const float danger = 1.0f - lowestHpPercent;
        const float softenedDanger = danger * m_heartbeatIntensityScale;
        visualHealth = 1.0f - softenedDanger;
    }

    PostProcessAPI::setHealth(visualHealth);

    float separation01 = 0.0f;

    if (m_bound != nullptr)
    {
        Transform* firstTransform = m_bound->m_firstTarget.getReferencedComponent();
        Transform* secondTransform = m_bound->m_secondTarget.getReferencedComponent();

        if (firstTransform != nullptr && secondTransform != nullptr && m_bound->m_minDistance > 0.0f)
        {
            const Vector3 firstPosition = TransformAPI::getGlobalPosition(firstTransform);
            const Vector3 secondPosition = TransformAPI::getGlobalPosition(secondTransform);
            const float distance = Vector3::Distance(firstPosition, secondPosition);

            const float excess = (distance - m_bound->m_minDistance) / m_bound->m_minDistance;
            separation01 = (excess > 0.0f) ? min(excess, 1.0f) : 0.0f;
        }
    }

    PostProcessAPI::setSeparation(separation01);

    const bool allPlayersDead = (m_player1 != nullptr) && (m_player2 != nullptr) &&
        m_player1->isDead() && m_player2->isDead();

    PostProcessAPI::setDeathFadeActive(allPlayersDead);
}

void PlayerVitalsMonitor::findPlayers()
{
    const auto playerGameObjects = SceneAPI::findAllGameObjectsWithScript<PlayerDamageable>();

    for (GameObject* playerGameObject : playerGameObjects)
    {
        PlayerDamageable* playerDamageable = GameObjectAPI::findScript<PlayerDamageable>(playerGameObject);

        if (playerDamageable == nullptr || playerDamageable == m_player1 || playerDamageable == m_player2)
        {
            continue;
        }

        if (m_player1 == nullptr)
        {
            m_player1 = playerDamageable;
        }
        else if (m_player2 == nullptr)
        {
            m_player2 = playerDamageable;
        }
    }
}

void PlayerVitalsMonitor::findBound()
{
    const auto boundGameObjects = SceneAPI::findAllGameObjectsWithScript<Bound>();

    if (!boundGameObjects.empty())
    {
        m_bound = GameObjectAPI::findScript<Bound>(boundGameObjects.front());
    }
}

bool PlayerVitalsMonitor::allPlayersFound() const
{
    return m_player1 != nullptr && m_player2 != nullptr;
}

IMPLEMENT_SCRIPT(PlayerVitalsMonitor)
