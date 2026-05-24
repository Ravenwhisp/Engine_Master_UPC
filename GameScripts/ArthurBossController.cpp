#include "pch.h"
#include "ArthurBossController.h"
#include "ArthurDetectionAggro.h"
#include "ArthurAttackConfig.h"
#include "Damageable.h"
#include <cmath>

static const char* navAgentProfileNames[] =
{
	"PlayerNormal",
	"PlayerSpectral",
	"EnemyGround"
};

constexpr int navAgentProfileCount = 3;

IMPLEMENT_SCRIPT_FIELDS(ArthurBossController,
	SERIALIZED_ENUM_INT(m_enemyType, "Enemy Type", navAgentProfileNames, navAgentProfileCount),
	SERIALIZED_FLOAT(m_combatRange, "Combat Range", 0.0f, 50.0f, 0.1f),
	SERIALIZED_FLOAT(m_moveSpeed, "Move Speed", 0.0f, 50.0f, 0.1f),
	SERIALIZED_FLOAT(m_turnSpeed, "Turn Speed", 0.0f, 5.0f, 0.1f),
	SERIALIZED_FLOAT(m_intervalRepath, "Interval", 0.0f, 50.0f, 0.1f),
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled")
)

ArthurBossController::ArthurBossController(GameObject* owner)
	: Script(owner)
{
}

void ArthurBossController::Start()
{
	m_arthurDetectionAggro = GameObjectAPI::findScript<ArthurDetectionAggro>(getOwner());

	if (!m_arthurDetectionAggro)
	{
		Debug::error("ArthurDetectionAggro script not found!");
	}

	m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());

	if (!m_attackConfig)
	{
		Debug::error("ArthurAttackConfig script not found!");
	}
}

void ArthurBossController::drawGizmo()
{
	if (!m_debugEnabled)
	{
		return;
	}

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
	}

	updateAttackCooldowns(Time::getDeltaTime());

	updateBossPhase();
}

bool ArthurBossController::hasValidTarget() const
{
	if (!m_arthurDetectionAggro)
	{
		return false;
	}

	Transform* targetTransform = m_arthurDetectionAggro->getCurrentTarget();
	if (!targetTransform)
	{
		return false;
	}

	if (m_arthurDetectionAggro->isDowned(targetTransform))
	{
		return false;
	}

	return true;
}

void ArthurBossController::updateCurrentTarget()
{
	if (!m_arthurDetectionAggro)
	{
		m_arthurDetectionAggro = GameObjectAPI::findScript<ArthurDetectionAggro>(getOwner());
	}

	if (!m_arthurDetectionAggro)
	{
		m_currentTarget = nullptr;
		return;
	}

	m_currentTarget = m_arthurDetectionAggro->getCurrentTarget();
}

bool ArthurBossController::isTargetInCombatRange() const
{
	if (!m_currentTarget)
	{
		return false;
	}

	Vector3 ownerPosition = getOwner()->GetTransform()->getPosition();
	Vector3 targetPosition = m_currentTarget->getPosition();

	Vector3 difference = ownerPosition - targetPosition;
	difference.y = 0.0f;

	return difference.Length() <= m_combatRange;
}

float ArthurBossController::getDistanceToCurrentTarget() const
{
	if (!m_currentTarget)
	{
		return FLT_MAX; // max value
	}

	Vector3 ownerPosition = getOwner()->GetTransform()->getPosition();
	Vector3 targetPosition = m_currentTarget->getPosition();

	Vector3 difference = ownerPosition - targetPosition;
	difference.y = 0.0f;

	return difference.Length();
}

bool ArthurBossController::isDead() const
{
	Damageable* damageable = GameObjectAPI::findScript<Damageable>(getOwner());

	if (damageable && damageable->isDead())
	{
		return true;
	}

	return false;
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
	if (!m_attackConfig)
	{
		Debug::error("[ArthurBossController] AtrhurAttackConfig not found.");
		return;
	}

	m_chargingSlamCooldownTimer = m_attackConfig->m_chargingSlamCooldown;
}

void ArthurBossController::consumeSideSweepCooldown()
{
	if (!m_attackConfig)
	{
		Debug::error("[ArthurBossController] AtrhurAttackConfig not found.");
		return;
	}

	m_sideSweepCooldownTimer = m_attackConfig->m_sideSweepCooldown;
}

void ArthurBossController::consumeEarthHammerCooldown()
{
	if (!m_attackConfig)
	{
		Debug::error("[ArthurBossController] AtrhurAttackConfig not found.");
		return;
	}

	m_earthHammerCooldownTimer = m_attackConfig->m_earthHammerCooldown;
}

void ArthurBossController::clearPath()
{
	m_path.clear();
	m_hasPath = false;
	m_currentIndex = 0;
}

bool ArthurBossController::buildPathToTarget()
{
	if (!m_currentTarget)
	{
		return false;
	}

	Vector3 start = getOwner()->GetTransform()->getPosition();
	Vector3 end = getChasePosition();

	std::vector<Vector3> tempPath;
	tempPath.resize(m_maxPathPoints);

	int pointCount = NavigationAPI::findStraightPath(start, end, tempPath.data(), m_maxPathPoints, m_searchExtents, static_cast<NavAgentProfile>(m_enemyType));

	if (pointCount > 0)
	{
		clearPath();

		for (int i = 0; i < pointCount; ++i)
		{
			m_path.push_back(tempPath[i]);
		}

		m_currentIndex = 0;
		m_hasPath = true;
		return true;
	}

	clearPath();

	return false;
}

void ArthurBossController::followPath()
{
	if (!m_hasPath)
	{
		return;
	}

	if (m_currentIndex >= m_path.size())
	{
		clearPath();
		return;
	}

	Vector3 currentPosition = getOwner()->GetTransform()->getPosition();
	Vector3 targetPoint = m_path[m_currentIndex];
	Vector3 direction = targetPoint - currentPosition;

	float distance = direction.Length();

	if (distance < 0.1f)
	{
		m_currentIndex++;
		if (m_currentIndex >= m_path.size())
		{
			clearPath();
			return;
		}
		return;
	}

	direction.Normalize();
	rotateTowardsDirection(direction);
	float dt = Time::getDeltaTime();

	currentPosition += direction * m_moveSpeed * dt;

	TransformAPI::setPosition(getOwner()->GetTransform(), currentPosition);
}

Vector3 ArthurBossController::getChasePosition() const
{
	if (!m_currentTarget)
	{
		return getOwner()->GetTransform()->getPosition();
	}

	Vector3 ownerPos = getOwner()->GetTransform()->getPosition();
	Vector3 targetPos = m_currentTarget->getPosition();
	Vector3 direction = targetPos - ownerPos;

	float distance = direction.Length();

	if (distance < 0.001f)
	{
		return targetPos;
	}

	direction.Normalize();

	Vector3 chasePosition = targetPos - direction * (m_combatRange - 0.1f);

	return chasePosition;
}

void ArthurBossController::rotateTowardsDirection(const Vector3& direction)
{
	Vector3 desiredDirection = direction;
	desiredDirection.y = 0.0f;

	if (desiredDirection.LengthSquared() < 0.0001f)
	{
		return;
	}

	Vector3 currentForward = TransformAPI::getForward(getOwner()->GetTransform());
	currentForward.y = 0.0f;

	if (currentForward.LengthSquared() < 0.0001f)
	{
		return;
	}

	desiredDirection.Normalize();
	currentForward.Normalize();

	float dot = currentForward.Dot(desiredDirection);

	if (dot > 1.0f) dot = 1.0f;
	if (dot < -1.0f) dot = -1.0f;

	float angle = std::acos(dot);

	Vector3 cross = currentForward.Cross(desiredDirection);

	float sign = (cross.y > 0) ? 1.0f : -1.0f;

	Vector3 currentEulerRotation = TransformAPI::getEulerDegrees(getOwner()->GetTransform());
	float maxStep = (m_turnSpeed * 100) * Time::getDeltaTime();

	float step = 0.0f;
	float angleDeg = angle * RADIANS_TO_DEGREES;

	if (angleDeg > maxStep)
	{
		step = maxStep * sign;
	}
	else
	{
		step = angleDeg * sign;
	}

	currentEulerRotation.y += step;
	TransformAPI::setRotationEuler(getOwner()->GetTransform(), currentEulerRotation);
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

void ArthurBossController::faceCurrentTarget()
{
	if (!m_currentTarget)
	{
		return;
	}

	Vector3 ownerPos = getOwner()->GetTransform()->getPosition();
	Vector3 targetPos = m_currentTarget->getPosition();
	Vector3 direction = targetPos - ownerPos;

	rotateTowardsDirection(direction);
}

void ArthurBossController::setRecoveryDuration(float recoveryDuration)
{
	m_recoveryDuration = recoveryDuration;
}

bool ArthurBossController::areBothPlayersInEarthHammerRange() const
{
	if (!m_attackConfig || !m_arthurDetectionAggro)
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

	float earthHammerRange = m_attackConfig->m_earthHammerRadius;
	float lyrielDistance = (lyrielPosition - ownerPosition).Length();
	float deathDistance = (deathPosition - ownerPosition).Length();

	return lyrielDistance <= earthHammerRange && deathDistance <= earthHammerRange;
}

bool ArthurBossController::isTargetInChargingSlamRange() const
{
	if (!m_attackConfig)
	{
		return false;
	}
	
	float distance = getDistanceToCurrentTarget();

	return distance >= m_attackConfig->m_chargingSlamMinRange && distance <= m_attackConfig->m_chargingSlamMaxRange;
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
	if (!m_attackConfig)
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
	float rangeSquared = m_attackConfig->m_sideSweepRange * m_attackConfig->m_sideSweepRange;

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
	float minDot = std::cos(m_attackConfig->m_sideSweepHalfAngleDegrees * degreesToRadians);

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

	if (!m_arthurDetectionAggro || !m_attackConfig)
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

void ArthurBossController::resetRepathTimer()
{
	m_repathTimer = 0.0f;
}

void ArthurBossController::addToRepathTimer(float dt)
{
	m_repathTimer += dt;
}

bool ArthurBossController::shouldRepath() const
{
	return m_repathTimer >= m_intervalRepath;
}

IMPLEMENT_SCRIPT(ArthurBossController)