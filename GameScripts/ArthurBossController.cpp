#include "pch.h"
#include "ArthurBossController.h"

#include "ArthurDetectionAggro.h"
#include "ArthurAttackConfig.h"
#include "ArthurUI.h"
#include "ArthurSound.h"

#include "Damageable.h"
#include "MusicManager.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(ArthurBossController, EnemyBaseController,
	SERIALIZED_ASSET_REF(m_attackConfig, "Arthur Attack Config", AssetType::DATA_CONTAINER),
	SERIALIZED_FLOAT(m_combatRange, "Combat Range", 0.0f, 50.0f, 0.1f)
)

ArthurBossController::ArthurBossController(GameObject* owner) : EnemyBaseController(owner)
{
}

void ArthurBossController::Start()
{
	m_arthurDetectionAggro = GameObjectAPI::findScript<ArthurDetectionAggro>(getOwner());

	if (!m_arthurDetectionAggro)
	{
		Debug::error("ArthurDetectionAggro script not found!");
	}

	m_arthurUI = GameObjectAPI::findScript<ArthurUI>(getOwner());

	if (!m_arthurUI)
	{
		Debug::error("ArthurUI script not found.");
	}

	m_arthurSound = GameObjectAPI::findScript<ArthurSound>(getOwner());

	m_currentTarget = nullptr;
	m_deathTriggerSent = false;

	resetRepathTimer();
	clearPath();
}

void ArthurBossController::drawGizmo()
{
	if (m_path.size() < 2)
	{
		return;
	}

	const Vector3 cyan = { 0.0f, 1.0f, 1.0f };
	for (int i = 0; i < m_path.size() - 1; ++i)
	{
		DebugDrawAPI::drawLine(m_path[i], m_path[i + 1], cyan, 0, true);
	}
}

void ArthurBossController::Update()
{

	if (!m_hasStartedEncounter && m_arthurDetectionAggro && m_arthurDetectionAggro->hasAnyTargetInDetectionRange())
	{
		m_arthurDetectionAggro->startEncounter();
		m_hasStartedEncounter = true;
		if (m_arthurUI)
		{
			m_arthurUI->showHealthUI(true);
		}

		if (m_arthurSound)
		{
			m_arthurSound->playIntroRoar();
		}

		// Música de boss. Si Arthur está en su propia escena con el MusicManager
		// configurado a Level1_Boss, esto setea el mismo estado (inocuo).
		if (MusicManager* music = MusicManager::Get())
		{
			music->SetState_Level1Boss();
		}
	}

	// Arthur derrotado: la música vuelve a la capilla (una sola vez).
	if (m_hasStartedEncounter && !m_bossDefeated)
	{
		if (m_damageable == nullptr)
		{
			m_damageable = GameObjectAPI::findScript<Damageable>(getOwner());
		}

		if (m_damageable != nullptr && m_damageable->isDead())
		{
			m_bossDefeated = true;

			if (m_arthurSound)
			{
				m_arthurSound->stopAllLoops();   // kill galloping/footsteps before the roar
				m_arthurSound->playDeathRoar();
			}

			if (MusicManager* music = MusicManager::Get())
			{
				music->SetState_Level1Chapel();
			}
		}
	}

	updateAttackCooldowns(Time::getDeltaTime());

	updateBossPhase();
}

Transform* ArthurBossController::acquireCurrentTarget()
{
	if (!m_arthurDetectionAggro)
	{
		m_arthurDetectionAggro = GameObjectAPI::findScript<ArthurDetectionAggro>(getOwner());
	}

	if (!m_arthurDetectionAggro)
	{
		return nullptr;
	}

	return m_arthurDetectionAggro->getCurrentTarget();
}

bool ArthurBossController::isTargetDowned(Transform* target) const
{
	if (!m_arthurDetectionAggro || !target)
	{
		return true;
	}

	return m_arthurDetectionAggro->isDowned(target);
}

void ArthurBossController::setPhase(ArthurBossPhase phase)
{
	m_phase = phase;
}

void ArthurBossController::updateBossPhase()
{
	if (isPhase2())
	{
		return;
	}

	if (!m_arthurDetectionAggro || !m_arthurDetectionAggro->isAggro())
	{
		return;
	}

	Damageable* damageable = GameObjectAPI::findScript<Damageable>(getOwner());
	if (!damageable)
	{
		return;
	}

    if (damageable->getMaxHp() <= 0.0f)
    {
      return;
    }

	if (damageable->getCurrentHp() <= damageable->getMaxHp() * 0.5f)
	{
		setPhase(ArthurBossPhase::Phase2);

		if (m_arthurSound)
		{
			m_arthurSound->playPhase2Roar();
		}

		if (m_arthurUI)
		{
			m_arthurUI->updateHealthUIPhase();
		}
	}
}

void ArthurBossController::updateAttackCooldowns(float dt)
{
	m_chargingSlamCooldownTimer -= dt;
	m_sideSweepCooldownTimer -= dt;
	m_earthHammerCooldownTimer -= dt;

	if (m_chargingSlamCooldownTimer <= 0.0f) m_chargingSlamCooldownTimer = 0.0f;
	if (m_sideSweepCooldownTimer <= 0.0f) m_sideSweepCooldownTimer = 0.0f;
	if (m_earthHammerCooldownTimer <= 0.0f) m_earthHammerCooldownTimer = 0.0f;
}

void ArthurBossController::consumeChargingSlamCooldown()
{
	const ArthurAttackConfig* cfg = m_attackConfig.get();
	if (!cfg)
	{
		Debug::error("[ArthurBossController] AtrhurAttackConfig not found.");
		return;
	}

	m_chargingSlamCooldownTimer = cfg->m_chargingSlamCooldown;
}

void ArthurBossController::consumeSideSweepCooldown()
{
	const ArthurAttackConfig* cfg = m_attackConfig.get();
	if (!cfg)
	{
		Debug::error("[ArthurBossController] AtrhurAttackConfig not found.");
		return;
	}

	m_sideSweepCooldownTimer = cfg->m_sideSweepCooldown;
}

void ArthurBossController::consumeEarthHammerCooldown()
{
	const ArthurAttackConfig* cfg = m_attackConfig.get();
	if (!cfg)
	{
		Debug::error("[ArthurBossController] AtrhurAttackConfig not found.");
		return;
	}

	m_earthHammerCooldownTimer = cfg->m_earthHammerCooldown;
}

Transform* ArthurBossController::getNonFocusTarget() const
{
	if (!m_arthurDetectionAggro)
	{
		return nullptr;
	}

	if (!m_currentTarget)
	{
		return nullptr;
	}

	Transform* lyrielTransform = m_arthurDetectionAggro->getLyrielTransform();
	Transform* deathTransform = m_arthurDetectionAggro->getDeathTransform();

	if (m_currentTarget == lyrielTransform)
	{
		return deathTransform;
	}

	if (m_currentTarget == deathTransform)
	{
		return lyrielTransform;
	}

	return nullptr;
}

bool ArthurBossController::areBothPlayersInEarthHammerRange() const
{
	const ArthurAttackConfig* cfg = m_attackConfig.get();
	if (!cfg || !m_arthurDetectionAggro)
	{
		return false;
	}

	Transform* lyriel = m_arthurDetectionAggro->getLyrielTransform();
	Transform* death = m_arthurDetectionAggro->getDeathTransform();

	if (!lyriel || !death)
	{
		return false;
	}

	if (m_arthurDetectionAggro->isDowned(lyriel) || m_arthurDetectionAggro->isDowned(death))
	{
		return false;
	}

	Vector3 ownerPosition = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner()));
	Vector3 lyrielPosition = TransformAPI::getGlobalPosition(lyriel);
	Vector3 deathPosition = TransformAPI::getGlobalPosition(death);

	float earthHammerRange = cfg->m_earthHammerRadius;
	float lyrielDistance = (lyrielPosition - ownerPosition).Length();
	float deathDistance = (deathPosition - ownerPosition).Length();

	return lyrielDistance <= earthHammerRange && deathDistance <= earthHammerRange;
}

bool ArthurBossController::isTargetInChargingSlamRange() const
{
	const ArthurAttackConfig* cfg = m_attackConfig.get();
	if (!cfg)
	{
		return false;
	}
	
	float distance = getDistanceToCurrentTarget();

	return distance >= cfg->m_chargingSlamMinRange && distance <= cfg->m_chargingSlamMaxRange;
}

bool ArthurBossController::isCurrentTargetInsideHeavySwipeArea(float range, float halfAngleDegrees) const
{
	if (!m_currentTarget)
	{
		return false;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

	if (!ownerTransform)
	{
		return false;
	}

	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector3 targetPosition = TransformAPI::getGlobalPosition(m_currentTarget);
	Vector3 toTarget = targetPosition - ownerPosition;

	toTarget.y = 0.0f;

	float distance = toTarget.Length();

	if (distance > range)
	{
		return false;
	}

	if (distance <= 0.001f)
	{
		return true;
	}

	toTarget.Normalize();

	Vector3 forward = TransformAPI::getForward(ownerTransform);

	float dot = forward.Dot(toTarget);

	float angleRadians = acosf(dot);
	float angleDegreesToTarget = angleRadians * RADIANS_TO_DEGREES;

	return angleDegreesToTarget <= halfAngleDegrees;
}

bool ArthurBossController::isTargetInsideSideSweepZone(Transform* targetTransform, int side) const
{
	const ArthurAttackConfig* cfg = m_attackConfig.get();
	if (!cfg)
	{
		return false;
	}

	if (!targetTransform)
	{
		return false;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return false;
	}

	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

	Vector3 toTarget = targetPosition - ownerPosition;
	toTarget.y = 0.0f;

	float distanceSquared = toTarget.LengthSquared();
	float rangeSquared = cfg->m_sideSweepRange * cfg->m_sideSweepRange;

	if (distanceSquared > rangeSquared)
	{
		return false;
	}

	if (distanceSquared < 0.0001f)
	{
		return true;
	}

	toTarget.Normalize();

	Vector3 sweepDirection = getSideSweepDirection(side);
	if (sweepDirection.LengthSquared() < 0.0001f)
	{
		return false;
	}

	sweepDirection.Normalize();

	float dot = sweepDirection.Dot(toTarget);

	if (dot > 1.0f)
	{
		dot = 1.0f;
	}
	else if (dot < -1.0f)
	{
		dot = -1.0f;
	}

	constexpr float degreesToRadians = 3.14159265f / 180.0f;
	float minDot = std::cos(cfg->m_sideSweepHalfAngleDegrees * degreesToRadians);

	return dot >= minDot;
}

Vector3 ArthurBossController::getSideSweepDirection(int side) const
{
	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	Vector3 forward = TransformAPI::getForward(ownerTransform);
	forward.y = 0.0f;

	if (forward.LengthSquared() < 0.0001f)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	forward.Normalize();

	constexpr float halfPi = 3.14159265f * 0.5f;

	return rotateAroundY(forward, halfPi * static_cast<float>(side));
}

Vector3 ArthurBossController::rotateAroundY(const Vector3& vector, float radians) const
{
	const float cosAngle = std::cos(radians);
	const float sinAngle = std::sin(radians);

	return Vector3(vector.x * cosAngle + vector.z * sinAngle,vector.y, -vector.x * sinAngle + vector.z * cosAngle);
}

bool ArthurBossController::trySelectSideSweepSide()
{
	if (!m_arthurDetectionAggro)
	{
		m_arthurDetectionAggro = GameObjectAPI::findScript<ArthurDetectionAggro>(getOwner());
	}

	if (!m_arthurDetectionAggro || !m_attackConfig.get())
	{
		return false;
	}

	Transform* lyrielTransform = m_arthurDetectionAggro->getLyrielTransform();
	Transform* deathTransform = m_arthurDetectionAggro->getDeathTransform();

	const bool lyrielInLeft = isTargetInsideSideSweepZone(lyrielTransform, -1);
	const bool lyrielInRight = isTargetInsideSideSweepZone(lyrielTransform, 1);

	const bool deathInLeft = isTargetInsideSideSweepZone(deathTransform, -1);
	const bool deathInRight = isTargetInsideSideSweepZone(deathTransform, 1);

	const bool anyPlayerInLeft = lyrielInLeft || deathInLeft;
	const bool anyPlayerInRight = lyrielInRight || deathInRight;

	if (anyPlayerInLeft && !anyPlayerInRight)
	{
		m_selectedSideSweepSide = -1;
		return true;
	}

	if (anyPlayerInRight && !anyPlayerInLeft)
	{
		m_selectedSideSweepSide = 1;
		return true;
	}

	if (anyPlayerInLeft && anyPlayerInRight)
	{
		// If both sides have a valid target choose the focus target side.
		updateCurrentTarget();

		if (isTargetInsideSideSweepZone(m_currentTarget, -1))
		{
			m_selectedSideSweepSide = -1;
			return true;
		}

		if (isTargetInsideSideSweepZone(m_currentTarget, 1))
		{
			m_selectedSideSweepSide = 1;
			return true;
		}
	}

	return false;
}

IMPLEMENT_SCRIPT(ArthurBossController)