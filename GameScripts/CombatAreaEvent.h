#pragma once

#include "ScriptAPI.h"
#include "GameplayEventAction.h"
#include "ParticleLifecycle.h"

class GameplayEventTrigger;

class CombatAreaEvent : public GameplayEventAction
{
    DECLARE_SCRIPT(CombatAreaEvent)

public:
    explicit CombatAreaEvent(GameObject* owner);

    void Update() override;
    void OnGameStop() override;

    void executeEvent(GameplayEventTrigger* trigger) override;

    bool isActive() const { return m_isActive; }
    bool hasCompleted() const { return m_hasCompleted; }

    FieldList getExposedFields() const override;

private:
    struct BarricadeVisualSlot
    {
        GameObject* mistInstance = nullptr;
        GameObject* burstInstance = nullptr;
    };

    void closeArea();
    void openArea();

    void setBlockerState(const ComponentRef<Transform>& blockerTransformRef, bool blocked);
    void activateBarricadeVisuals(const ComponentRef<Transform>& visualsTransformRef, BarricadeVisualSlot& slot);
    void deactivateBarricadeVisuals(BarricadeVisualSlot& slot);

    void removeDeadEnemies();
    bool shouldRemoveEnemy(const ComponentRef<Transform>& enemyTransformRef) const;

public:
    std::vector<ComponentRef<Transform>> m_enemies;

    ComponentRef<Transform> m_entranceBlocker;
    ComponentRef<Transform> m_exitBlocker;

    ComponentRef<Transform> m_entranceVisuals;
    ComponentRef<Transform> m_exitVisuals;

private:
    std::vector<ComponentRef<Transform>> m_remainingEnemies;

    BarricadeVisualSlot m_entranceBarricadeVfx;
    BarricadeVisualSlot m_exitBarricadeVfx;
    ParticleLifecycle::TimedParticleTracker m_timedParticles;

    bool m_isActive = false;
    bool m_hasCompleted = false;

    static constexpr float kBarricadeBurstDuration = 3.0f;
};
