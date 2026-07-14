#pragma once

#include "ScriptAPI.h"
#include "GameplayEventAction.h"

class GameplayEventTrigger;

class CombatAreaEvent : public GameplayEventAction
{
    DECLARE_SCRIPT(CombatAreaEvent)

public:
    explicit CombatAreaEvent(GameObject* owner);

    void Update() override;

    void executeEvent(GameplayEventTrigger* trigger) override;

    FieldList getExposedFields() const override;

private:
    void closeArea();
    void openArea();

    void setBlockerState(const ComponentRef<Transform>& blockerTransformRef, bool blocked);

	void setVisualsState(const ComponentRef<Transform>& visualsTransformRef, bool active);

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

    bool m_isActive = false;
    bool m_hasCompleted = false;
};