#pragma once

#include "ScriptAPI.h"

#include <vector>

class PlayerTargetController : public Script
{
    DECLARE_SCRIPT(PlayerTargetController)

public:
    explicit PlayerTargetController(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_targetRange = 8.0f;

private:
    GameObject* m_currentTarget = nullptr;

private:
    void updateAutoTarget();
    GameObject* findNearestEnemyInRange() const;

};
