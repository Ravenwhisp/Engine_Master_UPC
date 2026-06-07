#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class RangedEnemyController;
class ArcherAttackConfig;
class AnimationComponent;

class ArcherSomersaultState : public StateMachineScript
{
    DECLARE_SCRIPT(ArcherSomersaultState)

public:
    explicit ArcherSomersaultState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void moveSomersault();
    void finishSomersault();

private:
    RangedEnemyController* m_archerController = nullptr;
    ArcherAttackConfig* m_attackConfig = nullptr;
    AnimationComponent* m_animation = nullptr;

    Vector3 m_escapeDirection = Vector3(0.0f, 0.0f, 0.0f);

    float m_stateTimer = 0.0f;
};