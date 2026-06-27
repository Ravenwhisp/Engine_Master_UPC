#include "pch.h"
#include "DefeatConditionManager.h"
#include "PlayerState.h"
#include "PlayerDownState.h"

IMPLEMENT_SCRIPT_FIELDS(DefeatConditionManager,
    SERIALIZED_COMPONENT_REF(m_player1Transform, "Player 1 Transform", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_player2Transform, "Player 2 Transform", ComponentType::TRANSFORM)
)

DefeatConditionManager::DefeatConditionManager(GameObject* owner)
    : Script(owner)
{
}

void DefeatConditionManager::Start()
{
    Transform* player1Transform = m_player1Transform.getReferencedComponent();
    Transform* player2Transform = m_player2Transform.getReferencedComponent();

    m_player1State = findPlayerStateFromReference(player1Transform);
    m_player2State = findPlayerStateFromReference(player2Transform);

    m_player1DownState = findPlayerDownStateFromReference(player1Transform);
    m_player2DownState = findPlayerDownStateFromReference(player2Transform);

    if (!m_player1State)
    {
        Debug::warn("DefeatConditionManager: Could not find PlayerState for Player 1.");
    }

    if (!m_player2State)
    {
        Debug::warn("DefeatConditionManager: Could not find PlayerState for Player 2.");
    }

    if (!m_player1DownState)
    {
        Debug::warn("DefeatConditionManager: Could not find PlayerDownState for Player 1.");
    }

    if (!m_player2DownState)
    {
        Debug::warn("DefeatConditionManager: Could not find PlayerDownState for Player 2.");
    }
}

void DefeatConditionManager::Update()
{
    if (m_hasTriggeredDefeat)
    {
        return;
    }

    if (!m_player1State || !m_player2State)
    {
        return;
    }

    if (m_defeatCountdownStarted)
    {
        m_defeatTimer += Time::getDeltaTime();

        if (m_defeatTimer >= m_defeatDelay)
        {
            triggerDefeat();
        }

        return;
    }

    const bool bothPlayersDowned = m_player1State->isDowned() && m_player2State->isDowned();

    if (!bothPlayersDowned)
    {
        return;
    }

    m_defeatCountdownStarted = true;
    m_defeatTimer = 0.0f;

    if (m_player1DownState)
    {
        m_player1DownState->enterDefeatedState();
    }

    if (m_player2DownState)
    {
        m_player2DownState->enterDefeatedState();
    }

    Debug::log("Both players are downed. Defeat countdown started.");
}

PlayerState* DefeatConditionManager::findPlayerStateFromReference(Transform* transform) const
{
    if (transform == nullptr)
    {
        return nullptr;
    }

    GameObject* player = ComponentAPI::getOwner(transform);
    if (player == nullptr)
    {
        return nullptr;
    }

    return GameObjectAPI::findScript<PlayerState>(player);
}

PlayerDownState* DefeatConditionManager::findPlayerDownStateFromReference(Transform* transform) const
{
    if (transform == nullptr)
    {
        return nullptr;
    }

    GameObject* player = ComponentAPI::getOwner(transform);
    if (player == nullptr)
    {
        return nullptr;
    }

    return GameObjectAPI::findScript<PlayerDownState>(player);
}

void DefeatConditionManager::triggerDefeat()
{
    m_hasTriggeredDefeat = true;

    SceneAPI::requestSceneChange("Lose_Scene");
}

IMPLEMENT_SCRIPT(DefeatConditionManager)

