#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class RangedEnemyController;
class AnimationComponent;

class EnemyIdleState : public StateMachineScript
{
    DECLARE_SCRIPT(EnemyIdleState)

public:
    explicit EnemyIdleState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    RangedEnemyController* m_archerController = nullptr;
    AnimationComponent* m_animation = nullptr;
};