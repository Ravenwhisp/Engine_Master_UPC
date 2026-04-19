#include "pch.h"
#include "EnemyDetectionAggro.h"
#include "Damageable.h"
#include "DeathCharacter.h" 

static const ScriptFieldInfo enemyDetectionAggroFields[] =
{
	{ "Detection Radius", ScriptFieldType::Float, offsetof(EnemyDetectionAggro, m_detectionRadius), { 0.0f, 50.0f, 0.1f } },
	{ "Target Lock Duration", ScriptFieldType::Float, offsetof(EnemyDetectionAggro, m_targetLockDuration), { 0.0f, 10.0f, 0.1f } },
	{ "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyDetectionAggro, m_debugEnabled) },
	{ "Player 1 Transform", ScriptFieldType::ComponentRef, offsetof(EnemyDetectionAggro, m_player1Transform), {}, {}, { ComponentType::TRANSFORM } },
	{ "Player 2 Transform", ScriptFieldType::ComponentRef, offsetof(EnemyDetectionAggro, m_player2Transform), {}, {}, { ComponentType::TRANSFORM } }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyDetectionAggro, enemyDetectionAggroFields)

EnemyDetectionAggro::EnemyDetectionAggro(GameObject* owner) : Script(owner) {}

void EnemyDetectionAggro::Start()
{
}

void EnemyDetectionAggro::Update()
{
	m_currentTime += Time::getDeltaTime();

	updateTargetLockTimer();
	updateTauntTimer();
	updateAggroState();
}

void EnemyDetectionAggro::drawGizmo()
{
	if (!m_debugEnabled)
	{
		return;
	}

	const Vector3 white = { 1.0f, 1.0f, 1.0f };
	const Vector3 red = { 1.0f, 0.0f, 0.0f };
	const Vector3 yellow = { 1.0f, 1.0f, 0.0f };
	const Vector3 cyan = { 0.0f, 1.0f, 1.0f };

	Vector3 debugPosition = getOwnerPosition() + Vector3(0.0f, 0.2f, 0.0f);

	DebugDrawAPI::drawCircle(debugPosition, Vector3(0.0f, 1.0f, 0.0f), white, m_detectionRadius, 24.0f, 0, true);

	if (m_currentTargetTransform)
	{
		Vector3 targetPosition = TransformAPI::getPosition(m_currentTargetTransform);
		if (m_canSeeTarget)
		{
			DebugDrawAPI::drawLine(debugPosition, targetPosition, red, 0, true);
		}
		else
		{
			DebugDrawAPI::drawLine(debugPosition, targetPosition, yellow, 0, true);
		}
	}

	if (m_isAggro && !m_canSeeTarget)
	{
		DebugDrawAPI::drawCross(m_lastKnownTargetPosition, 0.35f, 0, true);
		DebugDrawAPI::drawPoint(m_lastKnownTargetPosition, cyan, 4.0f, 0, true);
	}
}

void EnemyDetectionAggro::enterAggro(Transform* target)
{
	if (!target)
	{
		return;
	}

	m_isAggro = true;
	m_canSeeTarget = true;
	m_currentTargetTransform = target;
	m_lastKnownTargetPosition = TransformAPI::getPosition(target);
}

void EnemyDetectionAggro::updateAggroState()
{
	updateAggroEntries();

	if (isTaunted())
	{
		if (!isTransformAlive(m_tauntTargetTransform))
		{
			clearTaunt(m_tauntTargetTransform);
		}
		else
		{
			m_isAggro = true;
			m_canSeeTarget = true;
			m_currentTargetTransform = m_tauntTargetTransform;
			m_lastKnownTargetPosition = TransformAPI::getPosition(m_tauntTargetTransform);
			return;
		}
	}

	if (!m_isAggro)
	{
		Transform* initialTarget = selectClosestDetectedPlayer();

		if (initialTarget)
		{
			enterAggro(initialTarget);
			m_currentTargetLockTimer = 0.0f;
		}

		return;
	}

	const bool currentTargetStillDetected =
		(m_currentTargetTransform == m_player1Aggro.targetTransform && m_player1Aggro.isInDetectionRange) ||
		(m_currentTargetTransform == m_player2Aggro.targetTransform && m_player2Aggro.isInDetectionRange);

	if (currentTargetStillDetected)
	{
		m_canSeeTarget = true;
		m_lastKnownTargetPosition = TransformAPI::getPosition(m_currentTargetTransform);
	}
	else
	{
		m_canSeeTarget = false;
	}

	if (!isTargetLockActive())
	{
		Transform* reevaluatedTarget = selectReevaluatedTarget();

		if (reevaluatedTarget && reevaluatedTarget != m_currentTargetTransform)
		{
			m_currentTargetTransform = reevaluatedTarget;
			m_lastKnownTargetPosition = TransformAPI::getPosition(m_currentTargetTransform);
		}

		const bool player1Aggroing = isPlayer1Aggroing();
		const bool player2Aggroing = isPlayer2Aggroing();

		if (player1Aggroing || player2Aggroing)
		{
			startTargetLock();
		}
		else
		{
			m_currentTargetLockTimer = 0.0f;
		}
	}
}

void EnemyDetectionAggro::updateAggroEntries()
{
	Transform* player1 = getPlayer1Transform();
	Transform* player2 = getPlayer2Transform();

	m_player1Aggro.targetTransform = player1;
	m_player2Aggro.targetTransform = player2;

	m_player1Aggro.isInDetectionRange = isPlayer1InDetectionRange();
	m_player2Aggro.isInDetectionRange = isPlayer2InDetectionRange();

	m_player1Aggro.distanceToEnemy = getDistanceToPlayer1();
	m_player2Aggro.distanceToEnemy = getDistanceToPlayer2();
}

bool EnemyDetectionAggro::isTargetLockActive() const
{
	return m_currentTargetLockTimer > 0.0f;
}

void EnemyDetectionAggro::startTargetLock()
{
	m_currentTargetLockTimer = m_targetLockDuration;
}

void EnemyDetectionAggro::updateTargetLockTimer()
{
	if (isTargetLockActive())
	{
		m_currentTargetLockTimer -= Time::getDeltaTime();

		if (m_currentTargetLockTimer < 0.0f)
		{
			m_currentTargetLockTimer = 0.0f;
		}
	}
}

void EnemyDetectionAggro::updateTauntTimer()
{
	if (!isTaunted())
	{
		return;
	}

	m_tauntTimer -= Time::getDeltaTime();

	if (m_tauntTimer <= 0.0f)
	{
		clearTaunt(m_tauntTargetTransform);
	}
}

bool EnemyDetectionAggro::isTaunted() const
{
	return m_tauntTargetTransform != nullptr && m_tauntTimer > 0.0f;
}

Transform* EnemyDetectionAggro::selectClosestDetectedPlayer() const
{
	const bool player1InRange = m_player1Aggro.isInDetectionRange;
	const bool player2InRange = m_player2Aggro.isInDetectionRange;

	if (player1InRange && !player2InRange)
	{
		return m_player1Aggro.targetTransform;
	}

	if (!player1InRange && player2InRange)
	{
		return m_player2Aggro.targetTransform;
	}

	if (player1InRange && player2InRange)
	{
		if (m_player1Aggro.distanceToEnemy < m_player2Aggro.distanceToEnemy)
		{
			return m_player1Aggro.targetTransform;
		}
		else
		{
			return m_player2Aggro.targetTransform;
		}
	}

	return nullptr;
}

Transform* EnemyDetectionAggro::selectReevaluatedTarget() const
{
	const bool player1Aggroing = isPlayer1Aggroing();
	const bool player2Aggroing = isPlayer2Aggroing();

	if (player1Aggroing && !player2Aggroing)
	{
		return m_player1Aggro.targetTransform;
	}

	if (!player1Aggroing && player2Aggroing)
	{
		return m_player2Aggro.targetTransform;
	}

	if (player1Aggroing && player2Aggroing)
	{
		if (m_player1Aggro.distanceToEnemy < m_player2Aggro.distanceToEnemy)
		{
			return m_player1Aggro.targetTransform;
		}
		else
		{
			return m_player2Aggro.targetTransform;
		}
	}

	return m_currentTargetTransform;
}

void EnemyDetectionAggro::notifyPlayerAttackedEnemy(Transform* playerTransform)
{
	AggroEntry* entry = getAggroEntry(playerTransform);

	if (!entry)
	{
		return;
	}

	entry->lastAttackTime = m_currentTime;

	if (!m_isAggro)
	{
		enterAggro(playerTransform);
		startTargetLock();
	}
}

void EnemyDetectionAggro::applyTaunt(Transform* playerTransform, float durationSeconds)
{
	if (playerTransform == nullptr || durationSeconds <= 0.0f)
	{
		return;
	}

	m_tauntTargetTransform = playerTransform;
	m_tauntTimer = durationSeconds;
	m_currentTargetLockTimer = 0.0f;
	enterAggro(playerTransform);

	// While taunted, range is ignored and line-of-sight is intentionally skipped.
	// TODO: Require wall checks / line-of-sight before applying the taunt effect.
	AggroEntry* entry = getAggroEntry(playerTransform);
	if (entry != nullptr)
	{
		entry->lastAttackTime = m_currentTime;
	}
}

void EnemyDetectionAggro::clearTaunt(Transform* playerTransform)
{
	if (playerTransform != nullptr && playerTransform != m_tauntTargetTransform)
	{
		return;
	}

	const bool wasCurrentTarget = (m_currentTargetTransform == m_tauntTargetTransform);

	m_tauntTargetTransform = nullptr;
	m_tauntTimer = 0.0f;

	if (!wasCurrentTarget)
	{
		return;
	}

	Transform* fallbackTarget = selectClosestDetectedPlayer();
	if (fallbackTarget != nullptr)
	{
		m_currentTargetTransform = fallbackTarget;
		m_lastKnownTargetPosition = TransformAPI::getPosition(fallbackTarget);
		m_canSeeTarget = true;
		m_isAggro = true;
	}
	else
	{
		m_currentTargetTransform = nullptr;
		m_canSeeTarget = false;
		m_isAggro = false;
		m_currentTargetLockTimer = 0.0f;
	}
}

Transform* EnemyDetectionAggro::getOwnerTransform() const
{
	return GameObjectAPI::getTransform(getOwner());
}

Transform* EnemyDetectionAggro::getPlayer1Transform() const
{
	return m_player1Transform.getReferencedComponent();
}

Transform* EnemyDetectionAggro::getPlayer2Transform() const
{
	return m_player2Transform.getReferencedComponent();
}

Vector3 EnemyDetectionAggro::getOwnerPosition() const
{
	Transform* ownerTransform = getOwnerTransform();
	if (!ownerTransform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	return TransformAPI::getPosition(ownerTransform);
}

Vector3 EnemyDetectionAggro::getPlayer1Position() const
{
	Transform* player1Transform = getPlayer1Transform();
	if (!player1Transform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	return TransformAPI::getPosition(player1Transform);
}

Vector3 EnemyDetectionAggro::getPlayer2Position() const
{
	Transform* player2Transform = getPlayer2Transform();
	if (!player2Transform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	return TransformAPI::getPosition(player2Transform);
}

float EnemyDetectionAggro::getDistanceToPlayer1() const
{
	Vector3 difference = getPlayer1Position() - getOwnerPosition();
	return difference.Length();
}

float EnemyDetectionAggro::getDistanceToPlayer2() const
{
	Vector3 difference = getPlayer2Position() - getOwnerPosition();
	return difference.Length();
}

bool EnemyDetectionAggro::isPlayer1InDetectionRange() const
{
	if (!getPlayer1Transform())
	{
		return false;
	}

	return getDistanceToPlayer1() <= m_detectionRadius;
}

bool EnemyDetectionAggro::isPlayer2InDetectionRange() const
{
	if (!getPlayer2Transform())
	{
		return false;
	}

	return getDistanceToPlayer2() <= m_detectionRadius;
}

bool EnemyDetectionAggro::isPlayer1Aggroing() const
{
	if (!m_player1Aggro.targetTransform)
	{
		return false;
	}

	return (m_currentTime - m_player1Aggro.lastAttackTime) <= m_recentAttackMemory;
}

bool EnemyDetectionAggro::isPlayer2Aggroing() const
{
	if (!m_player2Aggro.targetTransform)
	{
		return false;
	}

	return (m_currentTime - m_player2Aggro.lastAttackTime) <= m_recentAttackMemory;
}

bool EnemyDetectionAggro::isTransformAlive(Transform* target) const
{
	if (target == nullptr)
	{
		return false;
	}

	GameObject* targetOwner = ComponentAPI::getOwner(target);
	if (targetOwner == nullptr)
	{
		return false;
	}

	Script* script = GameObjectAPI::getScript(targetOwner, "Damageable");
	if (script == nullptr)
	{
		script = GameObjectAPI::getScript(targetOwner, "DeathCharacter");
	}

	Damageable* damageable = static_cast<Damageable*>(script);
	return damageable == nullptr || !damageable->isDead();
}

EnemyDetectionAggro::AggroEntry* EnemyDetectionAggro::getAggroEntry(Transform* target)
{
	if (target == getPlayer1Transform())
	{
		return &m_player1Aggro;
	}
	if (target == getPlayer2Transform())
	{
		return &m_player2Aggro;
	}
	return nullptr;
}

const EnemyDetectionAggro::AggroEntry* EnemyDetectionAggro::getAggroEntry(Transform* target) const
{
	if (target == getPlayer1Transform())
	{
		return &m_player1Aggro;
	}
	if (target == getPlayer2Transform())
	{
		return &m_player2Aggro;
	}
	return nullptr;
}

IMPLEMENT_SCRIPT(EnemyDetectionAggro)