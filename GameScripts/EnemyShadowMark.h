#pragma once

#include "ScriptAPI.h"

class EnemyShadowMark : public Script
{
    DECLARE_SCRIPT(EnemyShadowMark)

public:
    explicit EnemyShadowMark(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

    void notifyDeathHit();
    bool isExploitable() const { return m_phase == 3; }
    void exploit();
    int  getPhase() const { return m_phase; }

public:
    float m_markDuration = 3.0f;

private:
    int   m_phase = 0;
    float m_timer = 0.0f;
};
