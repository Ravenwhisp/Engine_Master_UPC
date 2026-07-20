#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"
#include "UISlider.h"

class ArthurUI : public Script
{
	DECLARE_SCRIPT(ArthurUI)

public:
	explicit ArthurUI(GameObject* owner);

	void Start() override;
	void Update() override;

	FieldList getExposedFields() const override;

public:
	// Health
	void setupHealthUI();
	void updateHealthUI();
	void showHealthUI(bool show);
	void updateHealthUIPhase();

	// Heavy Swipe
	void setupHeavySwipeUI();
	void updateHeavySwipeUI(float stateTimer, bool isPhase2, float hit1Time, float hit2Time, float hit3Time, float hit4Time, float totalDuration, float heavySwipeRange);
	void hideHeavySwipeUI();

	// Side Sweep
	void setupSideSweepUI(int sweepSide);
	void updateSideSweepUI(float stateTimer, float hitTime, float totalDuration);
	void hideSideSweepUI();

	// Charging Slam
	void setupChargingSlamUI(const Vector3& startPosition, const Vector3& lockedTargetPosition, const Vector3& dashDirection);
	void updateChargingSlamUI(float stateTimer, bool isPhase2, bool hasStartedDash, bool hasAppliedImpact, const Vector3& startPosition, const Vector3& lockedTargetPosition, float chargeDuration);
	void startChargingSlamImpactUI();
	void hideChargingSlamUI();

	// Earth Hammer
	void setupEarthHammerUI();
	void updateEarthHammerUI(float stateTimer, bool hasAppliedImpact, float hitTime, float impactDuration);
	void hideEarthHammerUI();

private:
	void applyHeavySwipeHitEffects(float t, Transform2D* glow, Transform2D* border, Transform2D* claw, float heavySwipeRange);

public:
	float m_healthBarDuration = 1.0f;

private:
	// Health
	ComponentRef<Transform> m_healthBarCanvas;
	ComponentRef<Transform2D> m_healthBarContainer;
	ComponentRef<Transform2D> m_healthBarPhase2;

	Transform* m_healthBarCanvasTransform = nullptr;
	Transform2D* m_healthBarContainerTransform2D = nullptr;
	Transform2D* m_healthBarPhase2Transform2D = nullptr;

	float m_healthBarTimer = 0.0f;
	bool m_healthBarVisible = false;

	float m_healthBarPhase2Timer = 0.0f;
	bool m_healthBarPhase2Visible = false;

	// Heavy Swipe
	ComponentRef<Transform> m_heavySwipeUICanvas;
	ComponentRef<Transform2D> m_heavySwipeUIContainer;
	ComponentRef<Transform2D> m_heavySwipeUIBackground;
	ComponentRef<Transform2D> m_heavySwipeUIBorder;
	ComponentRef<Transform2D> m_heavySwipeUIGlow;
	ComponentRef<Transform2D> m_heavySwipeUIRightClaw;
	ComponentRef<Transform2D> m_heavySwipeUILeftClaw;

	Transform* m_heavySwipeUICanvasTransform = nullptr;
	Transform2D* m_heavySwipeUIContainerTransform2D = nullptr;
	Transform2D* m_heavySwipeUIBackgroundTransform2D = nullptr;
	Transform2D* m_heavySwipeUIBorderTransform2D = nullptr;
	Transform2D* m_heavySwipeUIGlowTransform2D = nullptr;
	Transform2D* m_heavySwipeUIRightClawTransform2D = nullptr;
	Transform2D* m_heavySwipeUILeftClawTransform2D = nullptr;

	//Side Sweep
	ComponentRef<Transform> m_sideSweepUICanvas;
	ComponentRef<Transform2D> m_sideSweepUIContainer;
	ComponentRef<Transform2D> m_sideSweepUIBackground;
	ComponentRef<Transform2D> m_sideSweepUIShadow;

	Transform* m_sideSweepUICanvasTransform = nullptr;
	Transform2D* m_sideSweepUIContainerTransform2D = nullptr;
	Transform2D* m_sideSweepUIBackgroundTransform2D = nullptr;
	Transform2D* m_sideSweepUIShadowTransform2D = nullptr;

	// Charging Slam
	ComponentRef<Transform> m_chargingSlamUICanvas;
	ComponentRef<Transform2D> m_chargingSlamUIContainer;
	ComponentRef<Transform2D> m_chargingSlamUIBackground;
	ComponentRef<Transform2D> m_chargingSlamUIBorders;
	ComponentRef<Transform2D> m_chargingSlamUIShadow;
	ComponentRef<Transform2D> m_chargingSlamUISpikes;
	ComponentRef<UISlider> m_chargingSlamUIBordersSlider;
	ComponentRef<UISlider> m_chargingSlamUIShadowSlider;

	Transform* m_chargingSlamUICanvasTransform = nullptr;
	Transform2D* m_chargingSlamUIContainerTransform2D = nullptr;
	Transform2D* m_chargingSlamUIBackgroundTransform2D = nullptr;
	Transform2D* m_chargingSlamUIBordersTransform2D = nullptr;
	Transform2D* m_chargingSlamUIShadowTransform2D = nullptr;
	Transform2D* m_chargingSlamUISpikesTransform2D = nullptr;
	UISlider* m_chargingSlamUIBordersSliderComponent = nullptr;
	UISlider* m_chargingSlamUIShadowSliderComponent = nullptr;

	ComponentRef<Transform> m_chargingSlamImpactUICanvas;
	ComponentRef<Transform2D> m_chargingSlamImpactUIContainer;
	ComponentRef<Transform2D> m_chargingSlamImpactUICenter;
	ComponentRef<Transform2D> m_chargingSlamImpactUIGlow;

	Transform* m_chargingSlamImpactUICanvasTransform = nullptr;
	Transform2D* m_chargingSlamImpactUIContainerTransform2D = nullptr;
	Transform2D* m_chargingSlamImpactUICenterTransform2D = nullptr;
	Transform2D* m_chargingSlamImpactUIGlowTransform2D = nullptr;

	float m_chargingSlamUIFadeOutTimer = 0.0f;
	bool m_isChargingSlamUIFading = false;

	bool m_isChargingSlamImpactUIPlaying = false;
	float m_chargingSlamImpactUITimer = 0.0f;

	bool m_isChargingSlamImpactUIFading = false;
	float m_chargingSlamImpactUIFadeTimer = 0.0f;

	// Earth Hammer UI
	ComponentRef<Transform> m_earthHammerUICanvas;
	ComponentRef<Transform2D> m_earthHammerUIContainer;
	ComponentRef<Transform2D> m_earthHammerUIInner;
	ComponentRef<Transform2D> m_earthHammerUISpikes;
	ComponentRef<Transform2D> m_earthHammerUIGlow;
	ComponentRef<Transform2D> m_earthHammerUIRing;

	Transform* m_earthHammerUICanvasTransform = nullptr;
	Transform2D* m_earthHammerUIContainerTransform2D = nullptr;
	Transform2D* m_earthHammerUIInnerTransform2D = nullptr;
	Transform2D* m_earthHammerUISpikesTransform2D = nullptr;
	Transform2D* m_earthHammerUIGlowTransform2D = nullptr;
	Transform2D* m_earthHammerUIRingTransform2D = nullptr;

	bool m_earthHammerHasStartedImpactUI = false;
	float m_earthHammerImpactUITimer = 0.0f;
	float m_earthHammerInnerScale = 0.1f;
};
