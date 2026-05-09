#pragma once

#include "ScriptAPI.h"

class DebugGetObjectsArea : public Script
{
    DECLARE_SCRIPT(DebugGetObjectsArea)

public:
    explicit DebugGetObjectsArea(GameObject* owner);

    void Start() override;
    void Update() override;

	void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_radius = 5.0f;

};