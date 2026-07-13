#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class RangedEnemyController;
class ArcherAttackConfig;
class AnimationComponent;
class ArcherGuardParticles;

class ArcherSomersaultState : public StateMachineScript
{
    DECLARE_SCRIPT(ArcherSomersaultState)

public:
    explicit ArcherSomersaultState(GameObject* owner);

    FieldList getExposedFields() const override;

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void moveSomersault();
    void finishSomersault();

private:
    RangedEnemyController* m_archerController = nullptr;
    AssetRef<ArcherAttackConfig> m_attackConfig;
    AnimationComponent* m_animation = nullptr;
    ArcherGuardParticles* m_particles = nullptr;

    Vector3 m_escapeDirection = Vector3(0.0f, 0.0f, 0.0f);

    float m_stateTimer = 0.0f;
};