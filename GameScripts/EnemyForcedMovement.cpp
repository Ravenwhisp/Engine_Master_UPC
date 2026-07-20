#include "pch.h"
#include "EnemyForcedMovement.h"

#include "EnemyBaseController.h"
#include "EnemyDamageable.h"

EnemyForcedMovement::EnemyForcedMovement(GameObject* owner)
	: Script(owner)
{
}

void EnemyForcedMovement::Start()
{
	m_transform = GameObjectAPI::getTransform(getOwner());
	m_controller = GameObjectAPI::findScript<EnemyBaseController>(getOwner());
	m_damageable = GameObjectAPI::findScript<EnemyDamageable>(getOwner());

	if (!m_transform)
	{
		Debug::error("[EnemyForcedMovement] Transform not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	if (!m_controller)
	{
		Debug::error("[EnemyForcedMovement] EnemyBaseController not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	if (!m_damageable)
	{
		Debug::warn("[EnemyForcedMovement] EnemyDamageable not found on '%s'. Pull damage will not be applied.", GameObjectAPI::getName(getOwner()));
	}
}

void EnemyForcedMovement::Update()
{
	//Debug to see if it works
	/*if (Input::isKeyDown(KeyCode::U))
	{
		Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

		if (ownerTransform)
		{
			const Vector3 currentPosition = TransformAPI::getGlobalPosition(ownerTransform);
			Vector3 forward = TransformAPI::getForward(ownerTransform);
			forward.y = 0.0f;

			if (forward.LengthSquared() > 0.0001f)
			{
				forward.Normalize();

				const Vector3 destination = currentPosition + forward * 3.0f;

				startPull(destination, 1.0f, nullptr,	10.0f, PlayerAttackType::DeathTaunt
				);
			}
		}
	}*/

	if (m_isBeingPulled)
	{
		updatePull();
	}
}

bool EnemyForcedMovement::startPull(const Vector3& destination, float duration, Transform* attacker, float completionDamage, PlayerAttackType attackType)
{
	if (!m_transform || !m_controller)
	{
		return false;
	}

	if (m_controller->isDead())
	{
		return false;
	}

	if (m_controller->isForcedMovementBlocked())
	{
		Debug::log("[EnemyForcedMovement] Pull blocked on '%s'.", GameObjectAPI::getName(getOwner()));
		return false;
	}

	if (duration <= 0.0f)
	{
		return false;
	}

	m_startPosition = TransformAPI::getGlobalPosition(m_transform);
	m_destination = destination;

	m_elapsedTime = 0.0f;
	m_duration = duration;

	m_attacker = attacker;
	m_completionDamage = completionDamage;
	m_attackType = attackType;

	m_isBeingPulled = true;

	m_controller->setForcedMovementActive(true);

	return true;
}

void EnemyForcedMovement::cancelPull()
{
	if (!m_isBeingPulled)
	{
		return;
	}

	m_isBeingPulled = false;

	if (m_controller)
	{
		m_controller->setForcedMovementActive(false);
	}

	clearPullData();
}

void EnemyForcedMovement::updatePull()
{
	if (!m_transform || !m_controller)
	{
		cancelPull();
		return;
	}

	if (m_controller->isDead())
	{
		cancelPull();
		return;
	}

	if (m_duration <= 0.0f)
	{
		cancelPull();
		return;
	}

	m_elapsedTime += Time::getDeltaTime();

	float t = m_elapsedTime / m_duration;

	if (t < 0.0f)
	{
		t = 0.0f;
	}
	else if (t > 1.0f)
	{
		t = 1.0f;
	}

	const float easedT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInCubic, t);
	const Vector3 desiredPosition = m_startPosition + (m_destination - m_startPosition) * easedT;
	const Vector3 currentPosition = TransformAPI::getGlobalPosition(m_transform);

	Vector3 nextPosition;

	if (NavigationAPI::moveAlongSurface(currentPosition, desiredPosition, nextPosition, m_controller->m_pathSearchExtents))
	{
		TransformAPI::setGlobalPosition(m_transform, nextPosition);
	}

	if (t >= 1.0f)
	{
		finishPull();
	}
}

void EnemyForcedMovement::finishPull()
{
	if (!m_isBeingPulled)
	{
		return;
	}

	m_isBeingPulled = false;

	if (m_controller)
	{
		m_controller->setForcedMovementActive(false);
	}

	applyCompletionDamage();
	clearPullData();
}

void EnemyForcedMovement::applyCompletionDamage()
{
	if (!m_damageable)
	{
		return;
	}

	if (m_damageable->isDead())
	{
		return;
	}

	if (m_completionDamage <= 0.0f)
	{
		return;
	}

	EnemyHitContext hitContext;
	hitContext.damage = m_completionDamage;
	hitContext.attacker = m_attacker;
	hitContext.attackType = m_attackType;

	m_damageable->takeDamage(hitContext);
}

void EnemyForcedMovement::clearPullData()
{
	m_attacker = nullptr;

	m_startPosition = Vector3::Zero;
	m_destination = Vector3::Zero;

	m_elapsedTime = 0.0f;
	m_duration = 0.0f;
	m_completionDamage = 0.0f;

	m_attackType = PlayerAttackType::None;
}

IMPLEMENT_SCRIPT(EnemyForcedMovement)