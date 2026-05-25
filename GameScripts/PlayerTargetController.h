#pragma once

#include "ScriptAPI.h"
#include <vector>

class CharacterBase;

class PlayerTargetController : public Script
{
    DECLARE_SCRIPT(PlayerTargetController)

public:
    explicit PlayerTargetController(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

    GameObject* getCurrentTarget() const { return m_currentTarget; }

private:
    void updateTargetsInRange();
    void ensureValidCurrentTarget();
    void cycleTarget();

    bool isTargetInRange(GameObject* target) const;
    bool isTargetAlive(GameObject* target) const;
    int findTargetIndex(GameObject* target) const;

public:
    float m_targetRange = 8.0f;

private:
    CharacterBase* m_character = nullptr;
    GameObject* m_currentTarget = nullptr;
    std::vector<GameObject*> m_targetsInRange;
};