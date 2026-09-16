#include "pch.h"
#include "EnergyBallProjectile.h"

#include "Damageable.h"
#include "PlayerState.h"
#include "ProjectilePool.h"
#include "EnemyAttackExecutor.h"

IMPLEMENT_SCRIPT_FIELDS(EnergyBallProjectile,
	SERIALIZED_STRING(m_energyBallParticlePath, "Energy Ball Particle Prefab Path"),
	SERIALIZED_ASSET_REF(m_energyBallParticlePrefab, "Energy Ball Particle Prefab", AssetType::PREFAB)
)

EnergyBallProjectile::EnergyBallProjectile(GameObject* owner)
	: ProjectileBase(owner)
{
}

void EnergyBallProjectile::OnGameStop()
{
	removeEnergyBallParticle();
}

void EnergyBallProjectile::launch(const Vector3& startPosition, const Vector3& direction, float speed, float lifetime, GameObject* target, float damage)
{
	m_direction = direction;

	if (m_direction.LengthSquared() > 0.00001f)
	{
		m_direction.Normalize();
	}

	m_speed = speed;
	m_lifetime = lifetime;
	m_aliveTimer = 0.0f;
	m_target = target;
	m_damage = damage;
	m_isLaunched = true;
	m_inUse = true;

	GameObjectAPI::setActive(getOwner(), true);

	Transform* transform = GameObjectAPI::getTransform(getOwner());

	if (transform)
	{
		TransformAPI::setGlobalPosition(transform, startPosition);
		TransformAPI::lookAt(transform, startPosition + m_direction);
	}

	ensureEnergyBallParticle();
	updateEnergyBallParticle();

	if (m_energyBallParticle)
	{
		ParticleLifecycle::activate(m_energyBallParticle);
	}
}

void EnergyBallProjectile::resetProjectile()
{
	removeEnergyBallParticle();

	m_direction = Vector3::Zero;
	m_speed = 0.0f;
	m_lifetime = 0.0f;
	m_aliveTimer = 0.0f;
	m_damage = 0.0f;
	m_target = nullptr;
	m_isLaunched = false;

	ProjectileBase::resetProjectile();
}

void EnergyBallProjectile::Update()
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

	if (!m_target)
	{
		returnToPool();
		return;
	}

	Transform* projectileTransform = GameObjectAPI::getTransform(getOwner());
	Transform* targetTransform = GameObjectAPI::getTransform(m_target);

	if (!projectileTransform || !targetTransform)
	{
		returnToPool();
		return;
	}

	Vector3 projectilePosition = TransformAPI::getGlobalPosition(projectileTransform);
	Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

	Vector3 toTarget = targetPosition - projectilePosition;
	toTarget.y = 0.0f;

	const float distanceSquared = toTarget.LengthSquared();
	const float hitRadiusSquared = m_hitRadius * m_hitRadius;

	if (distanceSquared <= hitRadiusSquared)
	{
		applyImpactDamage();
		returnToPool();
		return;
	}

	if (distanceSquared <= 0.00001f)
	{
		return;
	}

	toTarget.Normalize();
	m_direction = toTarget;

	TransformAPI::translateGlobal(projectileTransform, m_direction * m_speed * Time::getDeltaTime());

	const Vector3 updatedPosition = TransformAPI::getGlobalPosition(projectileTransform);
	TransformAPI::lookAt(projectileTransform, updatedPosition + m_direction);

	updateEnergyBallParticle();
}

void EnergyBallProjectile::applyImpactDamage()
{
	if (!m_target)
	{
		return;
	}

	PlayerState* playerState = GameObjectAPI::findScript<PlayerState>(m_target);
	if (playerState && playerState->isDowned())
	{
		return;
	}

	Damageable* damageable = GameObjectAPI::findScript<Damageable>(m_target);
	if (!damageable)
	{
		return;
	}

	damageable->takeDamage(m_damage);

	Transform* targetTransform = GameObjectAPI::getTransform(m_target);
	if (m_pool && targetTransform)
	{
		GameObject* summonerObject = m_pool->getOwner();
		EnemyAttackExecutor* attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(summonerObject);

		if (attackExecutor)
		{
			attackExecutor->playPlayerHitVfx(targetTransform);
		}
	}

	Debug::log("[EnergyBallProjectile] Damaged target for %.2f.", m_damage);
}

void EnergyBallProjectile::ensureEnergyBallParticle()
{
	Transform* projectileTransform = GameObjectAPI::getTransform(getOwner());

	if (!projectileTransform)
	{
		return;
	}

	const Vector3 position = TransformAPI::getGlobalPosition(projectileTransform);
	const Vector3 rotation = TransformAPI::getGlobalEulerDegrees(projectileTransform);

	ParticleLifecycle::ensurePersistent(
		m_energyBallParticle,
		m_energyBallParticlePrefab.m_id,
		position,
		rotation,
		nullptr
	);

	if (m_energyBallParticle)
	{
		m_energyBallParticleTransform = GameObjectAPI::getTransform(m_energyBallParticle);
	}
}

void EnergyBallProjectile::updateEnergyBallParticle()
{
	Transform* projectileTransform = GameObjectAPI::getTransform(getOwner());

	if (!projectileTransform || !m_energyBallParticleTransform)
	{
		return;
	}

	const Vector3 position = TransformAPI::getGlobalPosition(projectileTransform);
	const Vector3 rotation = TransformAPI::getGlobalEulerDegrees(projectileTransform);

	TransformAPI::setGlobalPosition(m_energyBallParticleTransform, position);
	TransformAPI::setGlobalRotationEuler(m_energyBallParticleTransform, rotation);
}

void EnergyBallProjectile::removeEnergyBallParticle()
{
	ParticleLifecycle::deactivate(m_energyBallParticle);
	m_energyBallParticleTransform = nullptr;
}

IMPLEMENT_SCRIPT(EnergyBallProjectile)
