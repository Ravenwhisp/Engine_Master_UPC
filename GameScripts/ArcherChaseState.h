#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class RangedEnemyController;
class AnimationComponent;

class ArcherChaseState : public StateMachineScript
{
    DECLARE_SCRIPT(ArcherChaseState)

public:
    explicit ArcherChaseState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    RangedEnemyController* m_archerController = nullptr;
    AnimationComponent* m_animation = nullptr;
};