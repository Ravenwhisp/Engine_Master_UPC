#pragma once

#include "ScriptAPI.h"

class Transform;
class PlayerState;
class PlayerDownState;

class DefeatConditionManager : public Script
{
    DECLARE_SCRIPT(DefeatConditionManager)

public:
    explicit DefeatConditionManager(GameObject* owner);

    void Start() override;
    void Update() override;
    ScriptFieldList getExposedFields() const override;

    bool hasTriggeredDefeat() const { return m_hasTriggeredDefeat; }

private:
    PlayerState* findPlayerStateFromReference(Transform* transform) const;
    PlayerDownState* findPlayerDownStateFromReference(Transform* transform) const;

    void triggerDefeat();

public:
    ScriptComponentRef<Transform> m_player1Transform;
    ScriptComponentRef<Transform> m_player2Transform;

private:
    PlayerState* m_player1State = nullptr;
    PlayerState* m_player2State = nullptr;

    PlayerDownState* m_player1DownState = nullptr;
    PlayerDownState* m_player2DownState = nullptr;

    bool m_hasTriggeredDefeat = false;

    bool m_defeatCountdownStarted = false;
    float m_defeatTimer = 0.0f;
    float m_defeatDelay = 3.0f;
};