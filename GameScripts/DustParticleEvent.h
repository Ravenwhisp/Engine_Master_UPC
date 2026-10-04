#pragma once

#include "ScriptAPI.h"
#include "GameplayEventAction.h"
#include "ParticleLifecycle.h"

class GameplayEventTrigger;

class DustParticleEvent : public GameplayEventAction
{
    DECLARE_SCRIPT(DustParticleEvent)

public:
    explicit DustParticleEvent(GameObject* owner);

    void Update() override;
    void OnGameStop() override;

    void executeEvent(GameplayEventTrigger* trigger) override;

    FieldList getExposedFields() const override;

private:
    PrefabRef m_dustPrefab;
    std::vector<ComponentRef<Transform>> m_spawnPoints;

    float m_lifetime = 5.0f;

    ParticleLifecycle::TimedParticleTracker m_timedParticles;
};