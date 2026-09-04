#pragma once

#include "ProjectileBase.h"

class AelorinAttackExecutor;

class SeekerSigilProjectile : public ProjectileBase
{
	DECLARE_SCRIPT(SeekerSigilProjectile)

public:
	explicit SeekerSigilProjectile(GameObject* owner);

	void Update() override;

	void launch(const Vector3& startPosition, const Vector3& targetPosition, float fallSpeed, float lifetime, float impactRadius, float damage, AelorinAttackExecutor* attackExecutor);
	void resetProjectile() override;

private:
	void applyImpactDamage();

private:
	AelorinAttackExecutor* m_attackExecutor = nullptr;
	Vector3 m_targetPosition = Vector3::Zero;

	float m_fallSpeed = 0.0f;
	float m_lifetime = 0.0f;
	float m_aliveTimer = 0.0f;
	float m_damage = 0.0f;
	float m_impactRadius = 0.5f;

	bool m_isLaunched = false;
};