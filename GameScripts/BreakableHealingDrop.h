#pragma once

#include "ScriptAPI.h"

#include "BreakableObject.h"

class BreakableHealingDrop : public BreakableObject
{
    DECLARE_SCRIPT(BreakableHealingDrop)

public:
    explicit BreakableHealingDrop(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

public:
    PrefabRef m_healthPickupPrefab;

    float m_healthDropAmount = 10.0f;
    float m_dropRadius = 1.0f;
    float m_dropHeight = 1.0f;
    int m_healthDropQuantity = 1;

private:
    void onBreak() override;

};