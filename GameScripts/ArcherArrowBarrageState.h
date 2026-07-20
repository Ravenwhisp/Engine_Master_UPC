#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class RangedEnemyController;
class ArcherAttackConfig;
class EnemyAttackExecutor;
class AnimationComponent;
class ArcherUI;
class ArcherGuardParticles;
class ArcherSound;

class ArcherArrowBarrageState : public StateMachineScript
{
    DECLARE_SCRIPT(ArcherArrowBarrageState)

public:
    explicit ArcherArrowBarrageState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void lockImpactPosition();
    void applyImpact();
    void finishArrowBarrage();

private:
    RangedEnemyController* m_archerController = nullptr;
    AssetReference<ArcherAttackConfig> m_attackConfig;
    EnemyAttackExecutor* m_attackExecutor = nullptr;
    AnimationComponent* m_animation = nullptr;
    ArcherUI* m_archerUI = nullptr;
    ArcherGuardParticles* m_particles = nullptr;
    ArcherSound* m_archerSound = nullptr;

    Vector3 m_impactPosition = Vector3(0.0f, 0.0f, 0.0f);

    float m_stateTimer = 0.0f;

    bool m_hasLockedImpactPosition = false;
    bool m_hasAppliedImpact = false;
};