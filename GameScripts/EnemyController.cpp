#include "pch.h"
#include "EnemyController.h"
#include "EnemyDetectionAggro.h"
#include <cmath>

static const ScriptFieldInfo EnemyControllerFields[] =
{
	{ "Combat Range", ScriptFieldType::Float, offsetof(EnemyController, m_combatRange), { 0.0f, 50.0f, 0.1f } },
	{ "Move Speed", ScriptFieldType::Float, offsetof(EnemyController, m_moveSpeed), { 0.0f, 50.0f, 0.1f } },
	{ "Turn Speed", ScriptFieldType::Float, offsetof(EnemyController, m_turnSpeed), { 0.0f, 5.0f, 0.1f } },
	{ "Interval", ScriptFieldType::Float, offsetof(EnemyController, m_intervalRepath), { 0.0f, 50.0f, 0.1f } },
	{ "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyController, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyController, EnemyControllerFields)

EnemyController::EnemyController(GameObject* owner)
    : Script(owner)
{
}

void EnemyController::Start()
{

	Script* script = GameObjectAPI::getScript(m_owner, "EnemyDetectionAggro");
	m_enemyDetectionAggro = dynamic_cast<EnemyDetectionAggro*>(script);

	if (!m_enemyDetectionAggro)
	{
		Debug::error("EnemyDetectionAggro script not found!");
	}
}

void EnemyController::drawGizmo()
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

bool EnemyController::hasValidTarget() const
{
	if (m_enemyDetectionAggro->getCurrentTarget())
	{
		return true;
	}

	return false;
}

void EnemyController::updateCurrentTarget()
{
	if (!m_enemyDetectionAggro)
	{
		m_currentTarget = nullptr;
	}
	else
	{
		m_currentTarget = m_enemyDetectionAggro->getCurrentTarget();
	}
}

bool EnemyController::isTargetInCombatRange() const
{
	if (!m_currentTarget)
	{
		return false;
	}

	Vector3 distance = m_owner->GetTransform()->getPosition() - m_currentTarget->getPosition();

	if (distance.Length() <= m_combatRange)
	{
		return true;
	}

	return false;
}

void EnemyController::clearPath()
{
	m_path.clear();
	m_hasPath = false;
	m_currentIndex = 0;
}

bool EnemyController::buildPathToTarget()
{
	if (!m_currentTarget)
	{
		return false;
	}

	Vector3 start = m_owner->GetTransform()->getPosition();
	Vector3 end = getChasePosition();

	std::vector<Vector3> tempPath;
	tempPath.resize(m_maxPathPoints);

	int pointCount = NavigationAPI::findStraightPath(start, end, tempPath.data(), m_maxPathPoints, m_searchExtents);

	if (pointCount > 0)
	{
		clearPath();

		for (int i = 0; i < pointCount; ++i)
		{
			m_path.push_back(tempPath[i]);
		}

		// need to check if first point is current position or not
		m_currentIndex = 0;
		m_hasPath = true;
		return true;
	}
	
	clearPath();

	return false;
}

void EnemyController::followPath()
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

	Vector3 currentPosition = m_owner->GetTransform()->getPosition();
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

	TransformAPI::setPosition(m_owner->GetTransform(), currentPosition);	
}

Vector3 EnemyController::getChasePosition() const
{
	if (!m_currentTarget)
	{
		return m_owner->GetTransform()->getPosition();
	}

	Vector3 ownerPos = m_owner->GetTransform()->getPosition();
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

void EnemyController::rotateTowardsDirection(const Vector3& direction)
{
	Vector3 desiredDirection = direction;
	desiredDirection.y = 0.0f;

	if (desiredDirection.LengthSquared() < 0.0001f)
	{
		return;
	}

	Vector3 currentForward = TransformAPI::getForward(m_owner->GetTransform());
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

	Vector3 currentEulerRotation = TransformAPI::getEulerDegrees(m_owner->GetTransform());
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
	TransformAPI::setRotationEuler(m_owner->GetTransform(), currentEulerRotation);
}

void EnemyController::faceCurrentTarget()
{
	if (!m_currentTarget)
	{
		return;
	}
	
	Vector3 ownerPos = m_owner->GetTransform()->getPosition();
	Vector3 targetPos = m_currentTarget->getPosition();
	Vector3 direction = targetPos - ownerPos;

	rotateTowardsDirection(direction);
}

void EnemyController::resetRepathTimer()
{
	m_repathTimer = 0.0f;
}

void EnemyController::addToRepathTimer(float dt)
{
	m_repathTimer += dt;
}

bool EnemyController::shouldRepath() const
{
	return m_repathTimer >= m_intervalRepath;
}

IMPLEMENT_SCRIPT(EnemyController)