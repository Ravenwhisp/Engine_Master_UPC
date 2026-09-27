#pragma once

#include "ScriptAPI.h"
#include "UISheet.h"
#include "UISlider.h"
#include "Transform2D.h"

class CooperativeSound;

enum class ReaperGaugeVisualState
{
    Normal,
    FullEnter,
    FullIdle
};

class ReaperGauge : public Script
{
    DECLARE_SCRIPT(ReaperGauge)

public:
    explicit ReaperGauge(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void OnGameStop() override;
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
    int   m_numSegments      = 3;
    float m_gracePeriod      = 10.0f;
    float m_decayPerSecond   = 2.0f;

    ComponentRef<UISlider> m_reaperGaugeUI;
	ComponentRef<Transform2D> m_glowUI;
    ComponentRef<Transform2D> m_blinkAlphaUI;

    bool m_enableFullVfx = false;
    ComponentRef<Transform2D> m_fullGaugeContainer;
    ComponentRef<Transform2D> m_fullSegmentSurge1UI;
    ComponentRef<Transform2D> m_fullSegmentSurge2UI;
    ComponentRef<Transform2D> m_fullSegmentSurge3UI;
    ComponentRef<Transform2D> m_fullFrameEchoUI;

    AssetReference<void> m_fullLut;
    float m_fullLutFadeInDuration = 0.6f;
    float m_fullLutMinStrength = 0.7f;
    float m_fullLutMaxStrength = 1.0f;
    float m_fullLutBreathingSpeed = 1.25f;
    float m_fullEnterDuration = 0.5f;
    float m_fullCompressionScale = 0.98f;
    float m_fullPopScale = 1.06f;
    float m_fullBreathingSpeed = 2.0f;
    float m_fullBreathingIntensity = 0.18f;
    float m_fullSurgeInterval = 2.5f;
    float m_fullSurgeDuration = 1.05f;
    float m_fullSurgePeakAlpha = 0.55f;
    float m_fullFrameEchoScale = 1.025f;
    float m_fullFrameEchoAlpha = 0.28f;

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
    Transform2D* m_fullGaugeTransform = nullptr;
    Transform2D* m_fullSegmentSurgeTransforms[3] = { nullptr, nullptr, nullptr };
    Transform2D* m_fullFrameEchoTransform = nullptr;

    ReaperGaugeVisualState m_visualState = ReaperGaugeVisualState::Normal;
    Vector2 m_fullGaugeBaseScale = Vector2(1.0f, 1.0f);
    Vector2 m_fullSegmentSurgeBaseScales[3] = { Vector2(1.0f, 1.0f), Vector2(1.0f, 1.0f), Vector2(1.0f, 1.0f) };
    Vector2 m_fullFrameEchoBaseScale = Vector2(1.0f, 1.0f);
    float m_fullStateTimer = 0.0f;
    float m_surgeTimer = 0.0f;
    float m_surgeAnimTimer = 0.0f;
    float m_fullBarPulse = 0.0f;
    bool m_surgeAnimating = false;
    bool m_wasFull = false;

    bool m_lutCaptured = false;
    bool m_lutActive = false;
    bool m_previousLutEnabled = false;
    float m_previousLutStrength = 1.0f;
    float m_fullLutTimer = 0.0f;
    AssetId m_previousLutAsset;

    CooperativeSound* m_sound = nullptr;

    void beginFullVisuals(bool playAnnouncement);
    void endFullVisuals();
    void updateFullVisuals(float dt);
    void beginFullLut();
    void updateFullLut(float dt);
    void restorePreviousLut();
    void resetFullVisualComponents();
};
