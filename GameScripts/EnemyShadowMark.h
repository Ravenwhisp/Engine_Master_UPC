#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"
#include "PlayerAttackType.h"

class ReaperGauge;

enum class ShadowMarkState
{
    None = 0,
    DeathOnly,
    LyrielOnly,
    Ready
};

class EnemyShadowMark : public Script
{
    DECLARE_SCRIPT(EnemyShadowMark)

public:
    explicit EnemyShadowMark(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;

    FieldList getExposedFields() const override;

    virtual bool processAttack(PlayerAttackType attackType);
    bool isExploitable() const { return m_state == ShadowMarkState::Ready; }
    virtual void exploit();
    ShadowMarkState getState() const { return m_state; }
	void updateUI();
    void clearMark() { resetMark(); }

private:
    ReaperGauge* findReaperGauge();

    bool isDeathAttack(PlayerAttackType attackType) const;
    bool isLyrielAttack(PlayerAttackType attackType) const;

    bool canApplyWith(PlayerAttackType attackType) const;
    bool canExploitWith(PlayerAttackType attackType) const;

    void applyDeathContribution();
    void applyLyrielContribution();

    void resetTimer();
    void resetMark();

    // Effects
    void startExplosion();
    void updateExplosion();
    void restoreUIVisuals();

    void startEntryPop();
    void updateEntryPop();

public:
    bool m_useMarkDuration = true;
    float m_markDuration = 5.0f;
    float m_markFadeDuration = 0.5f;
    ComponentRef<Transform2D> m_canvas;
    ComponentRef<Transform> m_mark_death;
    ComponentRef<Transform> m_mark_lyriel;
    ComponentRef<Transform> m_mark_both;

    float m_volleyCooldownReduction = 0.20f;  // % of base cooldown removed per exploit

    // Effects
    float m_explosionDuration = 0.2f;
    float m_explosionScaleMultiplier = 1.3f;

    float m_entryPopDuration = 0.12f;
    float m_entryPopStartScaleMultiplier = 0.8f;
    float m_entryPopPeakScaleMultiplier = 1.1f;
    float m_readyPopPeakScaleMultiplier = 1.2f;

private:
    ShadowMarkState m_state = ShadowMarkState::None;
    float        m_timer        = 0.0f;
    ReaperGauge* m_reaperGauge  = nullptr;

    Transform2D* m_canvasTransform2D = nullptr;
	GameObject* m_mark1Object = nullptr;
    GameObject* m_mark2Object = nullptr;
	GameObject* m_mark3Object = nullptr;

    // Effects
    bool m_isExploding = false;
    float m_explosionTimer = 0.0f;
    Vector2 m_originalScale = { 1.0f, 1.0f };

    bool m_isEntryPopping = false;
    float m_entryPopTimer = 0.0f;
    float m_currentPopPeakMultiplier = 1.1f;

};

