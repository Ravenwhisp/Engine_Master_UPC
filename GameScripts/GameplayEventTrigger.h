#pragma once

#include "ScriptAPI.h"

class GameplayEventAction;

class GameplayEventTrigger : public Script
{
    DECLARE_SCRIPT(GameplayEventTrigger)

public:
    explicit GameplayEventTrigger(GameObject* owner);

    void Start() override;
    
    void OnTriggerEnter(GameObject* gameObject) override;
    void OnTriggerExit(GameObject* gameObject) override;

    ScriptFieldList getExposedFields() const override;

private:
    void findPlayers();
    void findEventActions();

    void setPlayerInside(GameObject* gameObject, bool inside);
    bool isTrackedPlayer(GameObject* gameObject) const;

    bool canActivate() const;
    void tryActivate();
    void activateEvent();

public:
    bool m_triggerOnlyOnce = true;

private:
    GameObject* m_player1 = nullptr;
    GameObject* m_player2 = nullptr;

    bool m_player1Inside = false;
    bool m_player2Inside = false;

    bool m_hasTriggered = false;

    std::vector<GameplayEventAction*> m_eventActions;
};