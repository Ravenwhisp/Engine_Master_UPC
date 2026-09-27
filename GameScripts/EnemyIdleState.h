#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyBaseController;
class SkeletonEnemyController;
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
    EnemyBaseController* m_controller = nullptr;
    SkeletonEnemyController* m_skeletonController = nullptr;
    AnimationComponent* m_animation = nullptr;
};
