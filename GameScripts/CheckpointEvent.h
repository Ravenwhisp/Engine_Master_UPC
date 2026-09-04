#pragma once

#include "ScriptAPI.h"

#include "GameplayEventAction.h"

#include "PersistingCheckpointState.h"

class GameplayEventTrigger;
class ReaperGauge;
class Damageable;

class CheckpointEvent : public GameplayEventAction
{
    DECLARE_SCRIPT(CheckpointEvent)

public:
    explicit CheckpointEvent(GameObject* owner);

    FieldList getExposedFields() const override;

	void executeEvent(GameplayEventTrigger* trigger) override;

    void Start() override;
    void Update() override;

protected:
	PersistingCheckpointState* m_PersistingCheckpointState = nullptr;
    ReaperGauge* m_reaperGauge = nullptr;
    Damageable* m_lyrielDamageable = nullptr;
	Damageable* m_deathDamageable = nullptr;

	CheckpointId m_checkpointId = CheckpointId::NONE;

private:
    ComponentRef<Transform> m_lyrielRespawn;
    ComponentRef<Transform> m_deathRespawn;

    Transform* m_lyrielRespawnTransform = nullptr;
    Transform* m_deathRespawnTransform = nullptr;

};