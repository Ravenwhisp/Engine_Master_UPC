#include "pch.h"
#include "DefeatConditionManager.h"
#include "PlayerState.h"

static const ScriptFieldInfo defeatConditionManagerFields[] =
{
    { "Player 1 Transform", ScriptFieldType::ComponentRef, offsetof(DefeatConditionManager, m_player1Transform), {}, {}, { ComponentType::TRANSFORM } },
    { "Player 2 Transform", ScriptFieldType::ComponentRef, offsetof(DefeatConditionManager, m_player2Transform), {}, {}, { ComponentType::TRANSFORM } }
};

IMPLEMENT_SCRIPT_FIELDS(DefeatConditionManager, defeatConditionManagerFields)

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

    if (!m_player1State)
    {
        Debug::warn("DefeatConditionManager: Could not find PlayerState for Player 1.");
    }

    if (!m_player2State)
    {
        Debug::warn("DefeatConditionManager: Could not find PlayerState for Player 2.");
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

    if (m_player1State->isDowned() && m_player2State->isDowned())
    {
        triggerDefeat();
    }
}

PlayerState* DefeatConditionManager::findPlayerStateFromReference(Transform* transform) const
{
    if (!transform)
    {
        return nullptr;
    }

    GameObject* player = ComponentAPI::getOwner(transform);
    if (!player)
    {
        return nullptr;
    }

    Script* script = GameObjectAPI::getScript(player, "PlayerState");
    return dynamic_cast<PlayerState*>(script);
}

void DefeatConditionManager::triggerDefeat()
{
    m_hasTriggeredDefeat = true;

    Debug::log("DefeatConditionManager: both players are downed. Defeat triggered.");
}

IMPLEMENT_SCRIPT(DefeatConditionManager)