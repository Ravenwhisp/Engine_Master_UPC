#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"
#include "UISlider.h"

class ArthurAttackConfig : public Script
{
    DECLARE_SCRIPT(ArthurAttackConfig)

public:
    explicit ArthurAttackConfig(GameObject* owner);

    void Start() override;

    ScriptFieldList getExposedFields() const override;

public:
    // Heavy Swipe
    float m_heavySwipeDamage = 10.0f;
    float m_heavySwipeRange = 3.0f;
    float m_heavySwipeHalfAngleDegrees = 60.0f;

    float m_heavySwipeTotalDuration = 1.5f;
    float m_heavySwipeHit1Time = 0.45f;
    float m_heavySwipeHit2Time = 0.95f;
    float m_heavySwipeHit3Time = 1.35f;

    float m_heavySwipeRecoveryDuration = 0.75f;

    float m_heavySwipePhase2Hit1Time = 0.30f;
    float m_heavySwipePhase2Hit2Time = 0.60f;
    float m_heavySwipePhase2Hit3Time = 0.90f;
    float m_heavySwipePhase2Hit4Time = 1.20f;
    float m_heavySwipePhase2RecoveryDuration = 0.45f;

    ScriptComponentRef<Transform> m_heavySwipeUICanvas;
    ScriptComponentRef<Transform2D> m_heavySwipeUIContainer;
    ScriptComponentRef<Transform2D> m_heavySwipeUIBackground;
    ScriptComponentRef<Transform2D> m_heavySwipeUIBorder;
    ScriptComponentRef<Transform2D> m_heavySwipeUIGlow;
    ScriptComponentRef<Transform2D> m_heavySwipeUIRightClaw;
    ScriptComponentRef<Transform2D> m_heavySwipeUILeftClaw;

	Transform* m_heavySwipeUICanvasTransform = nullptr;
	Transform2D* m_heavySwipeUIContainerTransform2D = nullptr;
	Transform2D* m_heavySwipeUIBackgroundTransform2D = nullptr;
	Transform2D* m_heavySwipeUIBorderTransform2D = nullptr;
	Transform2D* m_heavySwipeUIGlowTransform2D = nullptr;
	Transform2D* m_heavySwipeUIRightClawTransform2D = nullptr;
	Transform2D* m_heavySwipeUILeftClawTransform2D = nullptr;

    // Side Sweep
    float m_sideSweepDamage = 12.0f;
    float m_sideSweepRange = 4.0f;
    float m_sideSweepHalfAngleDegrees = 55.0f;

    float m_sideSweepHitTime = 0.35f;
    float m_sideSweepTotalDuration = 0.7f;

    float m_sideSweepRecoveryDuration = 0.5f;
    
    float m_sideSweepCooldown = 2.0f;

    float m_sideSweepPhase2HitTime = 0.20f;
    float m_sideSweepPhase2TotalDuration = 0.55f;
    float m_sideSweepPhase2RecoveryDuration = 0.35f;

    ScriptComponentRef<Transform> m_sideSweepUICanvas;
    ScriptComponentRef<Transform2D> m_sideSweepUIContainer;
    ScriptComponentRef<Transform2D> m_sideSweepUIBackground;
    ScriptComponentRef<Transform2D> m_sideSweepUIShadow;

    Transform* m_sideSweepUICanvasTransform = nullptr;
    Transform2D* m_sideSweepUIContainerTransform2D = nullptr;
    Transform2D* m_sideSweepUIBackgroundTransform2D = nullptr;
    Transform2D* m_sideSweepUIShadowTransform2D = nullptr;

    // Charging Slam
    float m_chargingSlamDashDamage = 12.0f;
    float m_chargingSlamFinalAreaImpactDamage = 25.0f;

    float m_chargingSlamHitTime = 1.0f;
    float m_chargingSlamDashSpeed = 8.0f;

    float m_chargingSlamDashHitRadius = 1.2f;
    float m_chargingSlamImpactRadius = 3.0f;

    float m_chargingSlamTotalDuration = 2.2f;
    float m_chargingSlamRecoveryDuration = 0.75f;

    float m_chargingSlamImpactStunDuration = 1.0f;
    
    float m_chargingSlamCooldown = 2.0f;

    float m_chargingSlamPhase2HitTime = 0.5f;
    float m_chargingSlamPhase2DashSpeed = 10.0f;

    float m_chargingSlamMinRange = 5.0f;
    float m_chargingSlamMaxRange = 8.0f;

    ScriptComponentRef<Transform> m_chargingSlamUICanvas;
    ScriptComponentRef<Transform2D> m_chargingSlamUIContainer;
	ScriptComponentRef<Transform2D> m_chargingSlamUIBackground;
    ScriptComponentRef<Transform2D> m_chargingSlamUIBorders;
    ScriptComponentRef<Transform2D> m_chargingSlamUIShadow;
    ScriptComponentRef<Transform2D> m_chargingSlamUISpikes;
    ScriptComponentRef<UISlider> m_chargingSlamUIBordersSlider;
    ScriptComponentRef<UISlider> m_chargingSlamUIShadowSlider;

    Transform* m_chargingSlamUICanvasTransform = nullptr;
	Transform2D* m_chargingSlamUIContainerTransform2D = nullptr;
    Transform2D* m_chargingSlamUIBackgroundTransform2D = nullptr;
    Transform2D* m_chargingSlamUIBordersTransform2D = nullptr;
    Transform2D* m_chargingSlamUIShadowTransform2D = nullptr;
	Transform2D* m_chargingSlamUISpikesTransform2D = nullptr;
	UISlider* m_chargingSlamUIBordersSliderComponent = nullptr;
    UISlider* m_chargingSlamUIShadowSliderComponent = nullptr;

	ScriptComponentRef<Transform> m_chargingSlamImpactUICanvas;
	ScriptComponentRef<Transform2D> m_chargingSlamImpactUIContainer;
    ScriptComponentRef<Transform2D> m_chargingSlamImpactUICenter;
    ScriptComponentRef<Transform2D> m_chargingSlamImpactUIGlow;

	Transform* m_chargingSlamImpactUICanvasTransform = nullptr;
	Transform2D* m_chargingSlamImpactUIContainerTransform2D = nullptr;
	Transform2D* m_chargingSlamImpactUICenterTransform2D = nullptr;
	Transform2D* m_chargingSlamImpactUIGlowTransform2D = nullptr;

    // Earth Hammer
    float m_earthHammerDamage = 20.0f;
    float m_earthHammerRadius = 5.0f;

    float m_earthHammerTotalDuration = 2.2f;
    float m_earthHammerHitTime = 2.0f;

    float m_earthHammerRecoveryDuration = 1.0f;

    float m_earthHammerStunDuration = 1.25f;
    
    float m_earthHammerCooldown = 2.0f;

    float m_earthHammerPhase2Damage = 25.0f;
    float m_earthHammerPhase2StunDuration = 1.75f;

	ScriptComponentRef<Transform> m_earthHammerUICanvas;
	ScriptComponentRef<Transform2D> m_earthHammerUIContainer;
    ScriptComponentRef<Transform2D> m_earthHammerUIInner;
    ScriptComponentRef<Transform2D> m_earthHammerUISpikes;
	ScriptComponentRef<Transform2D> m_earthHammerUIGlow;
    ScriptComponentRef<Transform2D> m_earthHammerUIRing;

	Transform* m_earthHammerUICanvasTransform = nullptr;
	Transform2D* m_earthHammerUIContainerTransform2D = nullptr;
    Transform2D* m_earthHammerUIInnerTransform2D = nullptr;
	Transform2D* m_earthHammerUISpikesTransform2D = nullptr;
    Transform2D* m_earthHammerUIGlowTransform2D = nullptr;
	Transform2D* m_earthHammerUIRingTransform2D = nullptr;
};