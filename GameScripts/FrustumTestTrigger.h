#pragma once

#include "ScriptAPI.h"

class BreakableObject;

class FrustumTestTrigger : public Script
{
    DECLARE_SCRIPT(FrustumTestTrigger)

public:
    explicit FrustumTestTrigger(GameObject* owner);

    void Start() override;
    void Update() override;
    FieldList getExposedFields() const override;

public:
    int m_playerIndex = 0;

private:
    BreakableObject* m_breakable = nullptr;
    bool m_prevSpaceDown = false;
};
