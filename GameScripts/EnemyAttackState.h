#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyBaseController;
class EnemyBaseAttackConfig;
class AnimationComponent;

class EnemyAttackState : public StateMachineScript
{
    DECLARE_SCRIPT(EnemyAttackState)

public:
    explicit EnemyAttackState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void tryDamageTarget(Transform* targetTransform);

private:
    EnemyBaseController* m_controller = nullptr;
    EnemyBaseAttackConfig* m_attackConfig = nullptr;
    AnimationComponent* m_animation = nullptr;

    Transform* m_committedTarget = nullptr;

    float m_stateTimer = 0.0f;
    bool m_hasAppliedDamage = false;
};