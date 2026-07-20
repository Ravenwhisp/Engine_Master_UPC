#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"

class ReaperGauge;

class EnemyShadowMark : public Script
{
    DECLARE_SCRIPT(EnemyShadowMark)

public:
    explicit EnemyShadowMark(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;

    FieldList getExposedFields() const override;

    void notifyDeathHit();
    bool isExploitable() const { return m_phase == 3; }
    virtual void exploit();
    int  getPhase() const { return m_phase; }
	void updateUI();

public:
    float m_markDuration              = 3.0f;
    float m_markUITargetScale = 1.0f;
	float m_markUIHeightOffset = 1.0f;
    ComponentRef<Transform2D> m_canvas;
    ComponentRef<Transform> m_mark_1;
    ComponentRef<Transform> m_mark_2;
    ComponentRef<Transform> m_mark_3;
    
    float m_volleyCooldownReduction   = 0.20f;  // % of base cooldown removed per exploit

private:
    ReaperGauge* findReaperGauge();

private:
    int          m_phase        = 0;
    float        m_timer        = 0.0f;
    ReaperGauge* m_reaperGauge  = nullptr;

    Transform2D* m_canvasTransform2D = nullptr;
	GameObject* m_mark1Object = nullptr;
    GameObject* m_mark2Object = nullptr;
	GameObject* m_mark3Object = nullptr;
	float m_startScale = 1.0f;

};

