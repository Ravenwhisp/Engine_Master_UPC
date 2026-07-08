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

	ScriptFieldList getExposedFields() const override;

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
	ScriptComponentRef<Transform> m_healthBarCanvas;
	ScriptComponentRef<Transform2D> m_healthBarContainer;
	ScriptComponentRef<Transform2D> m_healthBarPhase2;

	Transform* m_healthBarCanvasTransform = nullptr;
	Transform2D* m_healthBarContainerTransform2D = nullptr;
	Transform2D* m_healthBarPhase2Transform2D = nullptr;

	float m_healthBarTimer = 0.0f;
	bool m_healthBarVisible = false;

	float m_healthBarPhase2Timer = 0.0f;
	bool m_healthBarPhase2Visible = false;

	// Heavy Swipe
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

	//Side Sweep
	ScriptComponentRef<Transform> m_sideSweepUICanvas;
	ScriptComponentRef<Transform2D> m_sideSweepUIContainer;
	ScriptComponentRef<Transform2D> m_sideSweepUIBackground;
	ScriptComponentRef<Transform2D> m_sideSweepUIShadow;

	Transform* m_sideSweepUICanvasTransform = nullptr;
	Transform2D* m_sideSweepUIContainerTransform2D = nullptr;
	Transform2D* m_sideSweepUIBackgroundTransform2D = nullptr;
	Transform2D* m_sideSweepUIShadowTransform2D = nullptr;

	// Charging Slam
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

	float m_chargingSlamUIFadeOutTimer = 0.0f;
	bool m_isChargingSlamUIFading = false;

	bool m_isChargingSlamImpactUIPlaying = false;
	float m_chargingSlamImpactUITimer = 0.0f;

	bool m_isChargingSlamImpactUIFading = false;
	float m_chargingSlamImpactUIFadeTimer = 0.0f;

	// Earth Hammer UI
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

	bool m_earthHammerHasStartedImpactUI = false;
	float m_earthHammerImpactUITimer = 0.0f;
	float m_earthHammerInnerScale = 0.1f;
};
