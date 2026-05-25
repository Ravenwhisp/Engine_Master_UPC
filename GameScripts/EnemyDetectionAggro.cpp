#include "pch.h"
#include "EnemyDetectionAggro.h"
#include "PlayerState.h"
#include "Damageable.h"
#include "DeathCharacter.h" 

IMPLEMENT_SCRIPT_FIELDS(EnemyDetectionAggro,
	SERIALIZED_FLOAT(m_detectionRadius, "Detection Radius", 0.0f, 50.0f, 0.1f),
	SERIALIZED_FLOAT(m_targetLockDuration, "Target Lock Duration", 0.0f, 10.0f, 0.1f),
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled"),
	SERIALIZED_COMPONENT_REF(m_lyrielTransform, "Lyriel Transform", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_deathTransform, "Death Transform", ComponentType::TRANSFORM)
)

EnemyDetectionAggro::EnemyDetectionAggro(GameObject* owner) : Script(owner) {}

void EnemyDetectionAggro::Start()
{
	findPlayerTransforms();
}

void EnemyDetectionAggro::findPlayerTransforms()
{
	m_lyrielCachedTransform = m_lyrielTransform.getReferencedComponent();
	m_deathCachedTransform = m_deathTransform.getReferencedComponent();

	if (m_lyrielCachedTransform && m_deathCachedTransform)
		return;

	const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);
	for (GameObject* player : players)
	{
		const char* name = GameObjectAPI::getName(player);
		if (!name)
			continue;

		if (!m_lyrielCachedTransform && strcmp(name, "Lyriel") == 0)
			m_lyrielCachedTransform = GameObjectAPI::getTransform(player);

		if (!m_deathCachedTransform && strcmp(name, "Death") == 0)
			m_deathCachedTransform = GameObjectAPI::getTransform(player);

		if (m_lyrielCachedTransform && m_deathCachedTransform)
			break;
	}
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

	if (m_currentTargetTransform && isDowned(m_currentTargetTransform))
	{
		resetAggro();
	}

	if (isTaunted())
	{
		if (isDowned(m_tauntTargetTransform))
		{
			clearTaunt(m_tauntTargetTransform);
			return;
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
		!isDowned(m_currentTargetTransform) &&
		(
			(m_currentTargetTransform == m_lyrielAggro.targetTransform && m_lyrielAggro.isInDetectionRange) ||
			(m_currentTargetTransform == m_deathAggro.targetTransform && m_deathAggro.isInDetectionRange)
			);

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

		const bool lyrielAggroing = isLyrielAggroing();
		const bool deathAggroing = isDeathAggroing();

		if (lyrielAggroing || deathAggroing)
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
	Transform* lyriel = getLyrielTransform();
	Transform* death = getDeathTransform();

	m_lyrielAggro.targetTransform = lyriel;
	m_deathAggro.targetTransform = death;

	m_lyrielAggro.isInDetectionRange = isLyrielInDetectionRange();
	m_deathAggro.isInDetectionRange = isDeathInDetectionRange();

	m_lyrielAggro.distanceToEnemy = getDistanceToLyriel();
	m_deathAggro.distanceToEnemy = getDistanceToDeath();
}

void EnemyDetectionAggro::resetAggro()
{
	m_currentTargetTransform = nullptr;
	m_isAggro = false;
	m_canSeeTarget = false;
	m_currentTargetLockTimer = 0.0f;
	m_tauntTargetTransform = nullptr;
	m_tauntTimer = 0.0f;
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
	const bool lyrielInRange = m_lyrielAggro.isInDetectionRange && !isDowned(m_lyrielAggro.targetTransform);
	const bool deathInRange = m_deathAggro.isInDetectionRange && !isDowned(m_deathAggro.targetTransform);

	if (lyrielInRange && !deathInRange)
	{
		return m_lyrielAggro.targetTransform;
	}

	if (!lyrielInRange && deathInRange)
	{
		return m_deathAggro.targetTransform;
	}

	if (lyrielInRange && deathInRange)
	{
		if (m_lyrielAggro.distanceToEnemy < m_deathAggro.distanceToEnemy)
		{
			return m_lyrielAggro.targetTransform;
		}
		else
		{
			return m_deathAggro.targetTransform;
		}
	}

	return nullptr;
}

Transform* EnemyDetectionAggro::selectReevaluatedTarget() const
{
	const bool lyrielAggroing = isLyrielAggroing() && !isDowned(m_lyrielAggro.targetTransform);
	const bool deathAggroing = isDeathAggroing() && !isDowned(m_deathAggro.targetTransform);

	if (lyrielAggroing && !deathAggroing)
	{
		return m_lyrielAggro.targetTransform;
	}

	if (!lyrielAggroing && deathAggroing)
	{
		return m_deathAggro.targetTransform;
	}

	if (lyrielAggroing && deathAggroing)
	{
		if (m_lyrielAggro.distanceToEnemy < m_deathAggro.distanceToEnemy)
		{
			return m_lyrielAggro.targetTransform;
		}
		else
		{
			return m_deathAggro.targetTransform;
		}
	}

	if (!isDowned(m_currentTargetTransform))
	{
		return m_currentTargetTransform;
	}

	return selectClosestDetectedPlayer();
}

void EnemyDetectionAggro::notifyPlayerAttackedEnemy(Transform* playerTransform)
{
	if (isDowned(playerTransform))
	{
		return;
	}

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
		resetAggro();
	}
}

Transform* EnemyDetectionAggro::getOwnerTransform() const
{
	return GameObjectAPI::getTransform(getOwner());
}

Transform* EnemyDetectionAggro::getLyrielTransform() const
{
	Transform* ref = m_lyrielTransform.getReferencedComponent();
	return ref ? ref : m_lyrielCachedTransform;
}

Transform* EnemyDetectionAggro::getDeathTransform() const
{
	Transform* ref = m_deathTransform.getReferencedComponent();
	return ref ? ref : m_deathCachedTransform;
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

Vector3 EnemyDetectionAggro::getLyrielPosition() const
{
	Transform* lyrielTransform = getLyrielTransform();
	if (!lyrielTransform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	return TransformAPI::getPosition(lyrielTransform);
}

Vector3 EnemyDetectionAggro::getDeathPosition() const
{
	Transform* deathTransform = getDeathTransform();
	if (!deathTransform)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	return TransformAPI::getPosition(deathTransform);
}

float EnemyDetectionAggro::getDistanceToLyriel() const
{
	Vector3 difference = getLyrielPosition() - getOwnerPosition();
	return difference.Length();
}

float EnemyDetectionAggro::getDistanceToDeath() const
{
	Vector3 difference = getDeathPosition() - getOwnerPosition();
	return difference.Length();
}

bool EnemyDetectionAggro::isLyrielInDetectionRange() const
{
	if (!getLyrielTransform())
	{
		return false;
	}

	return getDistanceToLyriel() <= m_detectionRadius;
}

bool EnemyDetectionAggro::isDeathInDetectionRange() const
{
	if (!getDeathTransform())
	{
		return false;
	}

	return getDistanceToDeath() <= m_detectionRadius;
}

bool EnemyDetectionAggro::isLyrielAggroing() const
{
	if (!m_lyrielAggro.targetTransform)
	{
		return false;
	}

	return (m_currentTime - m_lyrielAggro.lastAttackTime) <= m_recentAttackMemory;
}

bool EnemyDetectionAggro::isDeathAggroing() const
{
	if (!m_deathAggro.targetTransform)
	{
		return false;
	}

	return (m_currentTime - m_deathAggro.lastAttackTime) <= m_recentAttackMemory;
}

bool EnemyDetectionAggro::isDowned(Transform* target) const
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

	PlayerState* state = GameObjectAPI::findScript<PlayerState>(targetOwner);
	if (!state)
	{
		return false;
	}

	return state->isDowned();
}

bool EnemyDetectionAggro::hasAnyTargetInDetectionRange()
{
	updateAggroEntries();

	return
		(m_lyrielAggro.isInDetectionRange && !isDowned(m_lyrielAggro.targetTransform)) ||
		(m_deathAggro.isInDetectionRange && !isDowned(m_deathAggro.targetTransform));
}

EnemyDetectionAggro::AggroEntry* EnemyDetectionAggro::getAggroEntry(Transform* target)
{
	if (target == getLyrielTransform())
	{
		return &m_lyrielAggro;
	}
	if (target == getDeathTransform())
	{
		return &m_deathAggro;
	}
	return nullptr;
}

const EnemyDetectionAggro::AggroEntry* EnemyDetectionAggro::getAggroEntry(Transform* target) const
{
	if (target == getLyrielTransform())
	{
		return &m_lyrielAggro;
	}
	if (target == getDeathTransform())
	{
		return &m_deathAggro;
	}
	return nullptr;
}

IMPLEMENT_SCRIPT(EnemyDetectionAggro)