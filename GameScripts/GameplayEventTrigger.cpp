#include "pch.h"
#include "GameplayEventTrigger.h"

#include "GameplayEventAction.h"

IMPLEMENT_SCRIPT_FIELDS(GameplayEventTrigger,
    SERIALIZED_BOOL(m_triggerOnlyOnce, "Trigger Only Once")
)

GameplayEventTrigger::GameplayEventTrigger(GameObject* owner)
    : Script(owner)
{
}

void GameplayEventTrigger::Start()
{
    findPlayers();
    findEventActions();
}

void GameplayEventTrigger::OnTriggerEnter(GameObject* gameObject)
{
    if (gameObject == nullptr)
    {
        return;
    }

    if (m_triggerOnlyOnce && m_hasTriggered)
    {
        return;
    }

    if (!isTrackedPlayer(gameObject))
    {
        return;
    }

    setPlayerInside(gameObject, true);
    tryActivate();
}

void GameplayEventTrigger::OnTriggerExit(GameObject* gameObject)
{
    if (gameObject == nullptr)
    {
        return;
    }

    if (!isTrackedPlayer(gameObject))
    {
        return;
    }

    const bool wasActive = m_isActive;

    setPlayerInside(gameObject, false);

    if (wasActive && !canActivate())
    {
        deactivateEvent();
    }
}

void GameplayEventTrigger::findPlayers()
{
    m_player1 = nullptr;
    m_player2 = nullptr;

    m_player1Inside = false;
    m_player2Inside = false;

    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER, true);

    if (!players.empty())
    {
        m_player1 = players[0];
    }

    if (players.size() > 1)
    {
        m_player2 = players[1];
    }

    if (m_player1 == nullptr || m_player2 == nullptr)
    {
        Debug::warn( "GameplayEventTrigger on '%s' requires two active players with tag PLAYER.", GameObjectAPI::getName(getOwner()));
    }
}

void GameplayEventTrigger::findEventActions()
{
    m_eventActions.clear();

    GameObject* owner = getOwner();
    if (owner == nullptr)
    {
        return;
    }

    const int scriptCount = GameObjectAPI::getScriptCount(owner);

    for (int i = 0; i < scriptCount; ++i)
    {
        Script* script = GameObjectAPI::getScriptByIndex(owner, i);
        if (script == nullptr)
        {
            continue;
        }

        GameplayEventAction* eventAction = dynamic_cast<GameplayEventAction*>(script);
        if (eventAction == nullptr)
        {
            continue;
        }

        m_eventActions.push_back(eventAction);
    }

    if (m_eventActions.empty())
    {
        Debug::warn("GameplayEventTrigger on '%s' has no GameplayEventAction scripts attached.", GameObjectAPI::getName(owner));
    }
}

void GameplayEventTrigger::setPlayerInside(GameObject* gameObject, bool inside)
{
    if (gameObject == m_player1)
    {
        m_player1Inside = inside;
        return;
    }

    if (gameObject == m_player2)
    {
        m_player2Inside = inside;
        return;
    }
}

bool GameplayEventTrigger::isTrackedPlayer(GameObject* gameObject) const
{
    return gameObject == m_player1 || gameObject == m_player2;
}

bool GameplayEventTrigger::canActivate() const
{
    return m_player1Inside && m_player2Inside;
}

void GameplayEventTrigger::tryActivate()
{
    if (m_isActive)
    {
        return;
    }

    if (m_triggerOnlyOnce && m_hasTriggered)
    {
        return;
    }

    if (!canActivate())
    {
        return;
    }

    m_isActive = true;

    if (m_triggerOnlyOnce)
    {
        m_hasTriggered = true;
    }

    activateEvent();
}

void GameplayEventTrigger::activateEvent()
{
    Debug::log("GameplayEventTrigger '%s' activated.", GameObjectAPI::getName(getOwner()));

    for (GameplayEventAction* eventAction : m_eventActions)
    {
        if (eventAction == nullptr)
        {
            continue;
        }

        eventAction->executeEvent(this);
    }
}

void GameplayEventTrigger::deactivateEvent()
{
    Debug::log("GameplayEventTrigger '%s' deactivated.", GameObjectAPI::getName(getOwner()));

    for (GameplayEventAction* eventAction : m_eventActions)
    {
        if (eventAction == nullptr)
        {
            continue;
        }

        eventAction->stopEvent(this);
    }

    m_isActive = false;
}

IMPLEMENT_SCRIPT(GameplayEventTrigger)