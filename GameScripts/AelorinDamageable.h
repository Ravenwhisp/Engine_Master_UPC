#pragma once

#include "EnemyDamageable.h"

#include <vector>

class AelorinBossController;

enum class AelorinThresholdType
{
	Standard,
	Fury,
	PhaseTransition,
	FinalDeath
};

struct AelorinThreshold
{
	float percent = 0.0f;
	AelorinThresholdType type = AelorinThresholdType::Standard;
};

class AelorinDamageable : public EnemyDamageable
{
	DECLARE_SCRIPT(AelorinDamageable)

public:
	explicit AelorinDamageable(GameObject* owner);

	void Start() override;

	void takeDamage(float amount) override;
	void takeDamage(const HitContext& ctx) override;

	bool isThresholdLocked() const { return m_thresholdLocked; }
	bool isPhaseTransitionPending() const { return m_phaseTransitionPending; }

	void beginPhase2();

protected:
	void onHpDepleted() override;

private:
	const std::vector<AelorinThreshold>& getActiveThresholds() const;

	const AelorinThreshold* getCurrentThreshold() const;
	bool hasCurrentThreshold() const;

	float getCurrentThresholdPercent() const;
	float getCurrentThresholdHp() const;

	void processNormalDamage(const EnemyHitContext& ctx);
	void processThresholdBreak(const EnemyHitContext& ctx);

	void lockCurrentThreshold();
	void advanceThreshold();
	void requestPhaseTransition();
	void handleFinalDeath(const EnemyHitContext& ctx);

private:
	AelorinBossController* m_controller = nullptr;

	std::vector<AelorinThreshold> m_phase1Thresholds
	{
		{ 0.50f, AelorinThresholdType::Standard },
		{ 0.00f, AelorinThresholdType::PhaseTransition }
	};

	std::vector<AelorinThreshold> m_phase2Thresholds
	{
		{ 0.70f, AelorinThresholdType::Standard },
		{ 0.45f, AelorinThresholdType::Fury },
		{ 0.25f, AelorinThresholdType::Standard },
		{ 0.10f, AelorinThresholdType::Fury },
		{ 0.00f, AelorinThresholdType::FinalDeath }
	};

	std::size_t m_currentThresholdIndex = 0;

	bool m_thresholdLocked = false;
	bool m_phaseTransitionPending = false;
	bool m_allowFinalDeath = false;
};