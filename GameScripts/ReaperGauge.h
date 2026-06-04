#pragma once

#include "ScriptAPI.h"
#include "UISlider.h"
#include "Transform2D.h"

class CooperativeSound;

class ReaperGauge : public Script
{
    DECLARE_SCRIPT(ReaperGauge)

public:
    explicit ReaperGauge(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;
    void updateUI();

    FieldList getExposedFields() const override;

    void  onMarkExploited();
    void  consume();
    float getGauge()        const { return m_gauge; }
    float getGaugePercent() const;
    int   getCurrentSegments() const;
    bool  isFull()          const { return m_gauge >= m_maxGauge; }

public:
    float m_maxGauge         = 100.0f;
    int   m_numSegments      = 5;
    float m_gainPerExploit   = 12.5f;
    float m_gracePeriod      = 10.0f;
    float m_decayPerSecond   = 2.0f;

    ScriptComponentRef<UISlider> m_reaperGaugeUI;
	ScriptComponentRef<Transform2D> m_glowUI;
    ScriptComponentRef<Transform2D> m_blinkAlphaUI;

	float m_blinkSpeed = 5.0f;
    float m_blinkAlpha = 0.25f;

private:
    float m_gauge         = 0.0f;
    float m_decayTimer    = 0.0f;
    bool  m_everExploited = false;
    bool  m_decaying      = false;
    
    UISlider* m_reaperGaugeSlider = nullptr;
	Transform2D* m_glowTransform = nullptr;
	Transform2D* m_blinkAlphaTransform = nullptr;

    CooperativeSound* m_sound = nullptr;
};
