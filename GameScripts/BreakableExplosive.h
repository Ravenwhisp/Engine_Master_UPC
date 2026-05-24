#pragma once

#include "ScriptAPI.h"

#include "BreakableObject.h"

class BreakableExplosive : public BreakableObject
{
    DECLARE_SCRIPT(BreakableExplosive)

public:
    explicit BreakableExplosive(GameObject* owner);

    void Start() override;
    void Update() override;

	void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_explosionRadius = 5.0f;
    float m_explosionDamage = 30.0f;

private:
	void onBreak() override;

};