#include "pch.h"
#include "SeekerSigilProjectile.h"

#include "AelorinAttackExecutor.h"

SeekerSigilProjectile::SeekerSigilProjectile(GameObject* owner)
	: ProjectileBase(owner)
{
}

void SeekerSigilProjectile::launch(const Vector3& startPosition, const Vector3& targetPosition,	float fallSpeed, float lifetime, float impactRadius, float damage, AelorinAttackExecutor* attackExecutor)
{
	m_targetPosition = targetPosition;

	m_fallSpeed = fallSpeed;
	m_lifetime = lifetime;
	m_aliveTimer = 0.0f;

	m_impactRadius = impactRadius;
	m_damage = damage;

	m_attackExecutor = attackExecutor;

	m_isLaunched = true;
	m_inUse = true;

	GameObjectAPI::setActive(getOwner(), true);

	Transform* projectileTransform = GameObjectAPI::getTransform(getOwner());

	if (!projectileTransform)
	{
		returnToPool();
		return;
	}

	TransformAPI::setGlobalPosition(projectileTransform, startPosition);
}

void SeekerSigilProjectile::Update()
{
	if (!m_isLaunched)
	{
		return;
	}

	m_aliveTimer += Time::getDeltaTime();

	if (m_aliveTimer >= m_lifetime)
	{
		returnToPool();
		return;
	}

	Transform* projectileTransform = GameObjectAPI::getTransform(getOwner());

	if (!projectileTransform)
	{
		returnToPool();
		return;
	}

	const Vector3 currentPosition = TransformAPI::getGlobalPosition(projectileTransform);

	Vector3 toTarget = m_targetPosition - currentPosition;

	const float distanceSquared = toTarget.LengthSquared();

	const float movementThisFrame =	m_fallSpeed * Time::getDeltaTime();

	if (distanceSquared <= movementThisFrame * movementThisFrame)
	{
		TransformAPI::setGlobalPosition(projectileTransform, m_targetPosition);

		applyImpactDamage();
		returnToPool();
		return;
	}

	if (distanceSquared <= 0.00001f)
	{
		applyImpactDamage();
		returnToPool();
		return;
	}

	toTarget.Normalize();

	TransformAPI::translateGlobal(projectileTransform, toTarget * movementThisFrame);
}

void SeekerSigilProjectile::applyImpactDamage()
{
	if (!m_attackExecutor)
	{
		Debug::warn("[SeekerSigilProjectile] AelorinAttackExecutor not found.");
		return;
	}

	m_attackExecutor->applyDamageInRadius(m_targetPosition, m_impactRadius, m_damage, "Aelorin Seeker Sigils");
}

void SeekerSigilProjectile::resetProjectile()
{
	m_targetPosition = Vector3::Zero;

	m_fallSpeed = 0.0f;
	m_lifetime = 0.0f;
	m_aliveTimer = 0.0f;

	m_impactRadius = 0.0f;
	m_damage = 0.0f;

	m_attackExecutor = nullptr;

	m_isLaunched = false;

	ProjectileBase::resetProjectile();
}

IMPLEMENT_SCRIPT(SeekerSigilProjectile)