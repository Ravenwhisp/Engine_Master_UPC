#pragma once

#include "Damageable.h"
#include "PlayerAttackType.h"

class EnemyDetectionAggro;
class EnemySound;
class EnemyShadowMark;
class Transform2D;
class EnemyBaseController;
class EnemyBaseDataConfig;
class ReaperGauge;
class ShadowExecution;
class UISlider;

struct EnemyHitContext : public HitContext
{
	Transform* attacker = nullptr;
	PlayerAttackType attackType = PlayerAttackType::None;
};

class EnemyDamageable : public Damageable
{
	DECLARE_SCRIPT(EnemyDamageable)

public:
	explicit EnemyDamageable(GameObject* owner);

	void Start() override;
	void Update() override;

	FieldList getExposedFields() const override;
	
    void takeDamage(const HitContext& ctx) override;
	void kill() override;
	bool lastHitExploitShadowMark() const { return m_lastHitExploitedShadowMark; }
	float getShadowExecutionThresholdMultiplier() const;

	void playShadowExecutionHitPreview();

	void startDissolve();
	bool isDissolveFinished() const;

protected:
	void onDamaged(float amount) override;
	void onDeath() override;

	void resetLastShadowMarkResult() { m_lastHitExploitedShadowMark = false; }
	bool processShadowMarkHit(PlayerAttackType attackType);
	void applyDamageWithoutShadowMark(const EnemyHitContext& hit);

	virtual void setHealthBarAlpha(float alpha);

	DissolveComponent* m_dissolve = nullptr;
	bool m_dissolveActive = false;
	float m_dissolveTimer = 0.0f;
	float m_dissolveDuration = 1.0f;
	void loadDissolveComponent();
	DissolveComponent* findDissolveInHierarchy(Transform* transform);
	
	Transform2D* getHealthBarContainerTransform() const { return m_healthBarContainerTransform; }
	void bindHealthBarUI(Transform2D* container, UISlider* slider1, UISlider* slider2);

private: 
	void resolveHealthBarReferences();
	void updateHealthBarFade();
	void updateDissolveEffect();

	void resolveReaperGauge();
	void updateShadowExecutionPreviewAvailability();
	void setShadowExecutionPreviewActive(bool active);

	void resolveShadowExecution();
	void updateShadowExecutionPreview();

	void updateShadowExecutionPreviewAnimation(float dt);
	void resetShadowExecutionPreviewVisual();

	void updateShadowExecutionThresholdMarker();
	void setShadowExecutionThresholdMarkerVisible(bool visible);

private:
	const EnemyBaseDataConfig* m_baseDataConfig = nullptr;
	EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
	EnemySound* m_enemySound = nullptr;
	EnemyShadowMark* m_shadowMark = nullptr;
	Transform* m_damageSource = nullptr;
	ReaperGauge* m_reaperGauge = nullptr;
	ShadowExecution* m_shadowExecution = nullptr;
	
	bool m_lastHitExploitedShadowMark = false;

	ComponentRef<Transform2D> m_healthBarContainer;
	Transform2D* m_healthBarContainerTransform = nullptr;

	float m_healthBarFadeTime = 0.25f;
	float m_healthBarFadeTimer = 0.0f;
	bool m_healthBarFadeActive = false;

	// Shadow Execution Health Bar effects
	bool m_shadowExecutionPreviewActive = false;

	ComponentRef<UISlider> m_shadowExecutionPreview;
	UISlider* m_shadowExecutionPreviewSlider = nullptr;
	Transform2D* m_shadowExecutionPreviewTransform = nullptr;

	Vector2 m_shadowExecutionPreviewBaseScale = Vector2(1.0f, 1.0f);

	float m_shadowExecutionPreviewFadeTimer = 0.0f;
	float m_shadowExecutionPreviewHitTimer = 0.0f;
	float m_shadowExecutionPreviewHitStart = 0.0f;
	float m_shadowExecutionPreviewHitEnd = 0.0f;

	bool m_shadowExecutionPreviewLethal = false;
	bool m_shadowExecutionPreviewHitAnimating = false;

	float m_shadowExecutionPreviewFadeTime = 0.2f;
	float m_shadowExecutionPreviewHitTime = 0.2f;
	float m_shadowExecutionPreviewNonLethalAlpha = 0.7f;
	float m_shadowExecutionPreviewLethalAlpha = 1.0f;

	ComponentRef<Transform2D> m_shadowExecutionThresholdMarker;
	Transform2D* m_shadowExecutionThresholdMarkerTransform = nullptr;
};
