#include "pch.h"
#include "SummonerEnergyBallState.h"

#include "SummonerEnemyController.h"
#include "SummonerAttackConfig.h"
#include "EnergyBallProjectile.h"
#include "ProjectilePool.h"

SummonerEnergyBallState::SummonerEnergyBallState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SummonerEnergyBallState::OnStateEnter()
{
    m_controller = GameObjectAPI::findScript<SummonerEnemyController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

	m_stateTimer = 0.0f;
	m_hasFiredEnergyBall = false;

	if (!m_controller)
	{
		Debug::error("[SummonerEnergyBallState] EnemyController not found.");
		return;
	}

    if (!m_animation)
	{
		Debug::error("[SummonerEnergyBallState] AnimationComponent not found.");
		return;
	}

	m_controller->updateCurrentTarget();
	m_committedTarget = m_controller->getCurrentTarget();

	Debug::log("[SummonerEnergyBallState] ENTER");
}

void SummonerEnergyBallState::OnStateUpdate()
{
    if (!m_controller || !m_animation)
    {
        return;
    }

    if (m_controller->trySendDeathTrigger(m_animation))
    {
        return;
    }

    const SummonerAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    m_controller->faceCurrentTarget();

    m_stateTimer += Time::getDeltaTime();

    if (!m_hasFiredEnergyBall && m_stateTimer >= cfg->m_basicAttackWindupTime)
	{
		spawnEnergyBall();
		m_controller->consumeAttackCooldown();
		m_hasFiredEnergyBall = true;
	}

    if (m_stateTimer >= cfg->m_basicAttackTotalDuration)
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}
}

void SummonerEnergyBallState::OnStateExit()
{
	m_stateTimer = 0.0f;
	m_hasFiredEnergyBall = false;

	Debug::log("[SummonerEnergyBallState] EXIT");
}

void SummonerEnergyBallState::spawnEnergyBall()
{
    if (!m_controller)
    {
        return;
    }

    const SummonerAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	Transform* targetTransform = m_controller->getCurrentTarget();

	if (!ownerTransform || !targetTransform)
	{
		return;
	}

	const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

	Vector3 direction = targetPosition - ownerPosition;
	direction.y = 0.0f;

	if (direction.LengthSquared() <= 0.00001f)
	{
		return;
	}

	direction.Normalize();

	const Vector3 spawnPosition = ownerPosition + direction;

	ProjectilePool* projectilePool = GameObjectAPI::findScript<ProjectilePool>(getOwner());
	if (!projectilePool)
	{
		Debug::error("[SummonerEnergyBallState] ProjectilePool not found.");
		return;
	}

	ProjectileBase* pooledProjectile = projectilePool->acquireProjectile();
	if (!pooledProjectile)
	{
		Debug::warn("[SummonerEnergyBallState] No available energy ball projectile.");
		return;
	}

	EnergyBallProjectile* projectile = static_cast<EnergyBallProjectile*>(pooledProjectile);

	GameObject* targetObject = targetTransform->getOwner();

    projectile->launch(
        spawnPosition,
        direction,
        cfg->m_energyBallSpeed,
        cfg->m_energyBallLifetime,
        targetObject,
        cfg->m_basicAttackDamage
    );
}

IMPLEMENT_SCRIPT_FIELDS(SummonerEnergyBallState,
    SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER)
)