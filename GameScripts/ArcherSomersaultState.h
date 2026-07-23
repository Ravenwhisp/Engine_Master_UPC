#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class RangedEnemyController;
class AnimationComponent;
class ArcherGuardParticles;

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
    void cancelSomersault();

private:
    RangedEnemyController* m_archerController = nullptr;
    AnimationComponent* m_animation = nullptr;
    ArcherGuardParticles* m_particles = nullptr;

    Vector3 m_escapeDirection = Vector3(0.0f, 0.0f, 0.0f);

    float m_stateTimer = 0.0f;
};