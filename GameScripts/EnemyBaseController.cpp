#include "pch.h"
#include "EnemyBaseController.h"
#include "EnemyBaseAttackConfig.h"

#include "Damageable.h"
#include "EnemyBaseDataConfig.h"
#include "EnemySound.h"
#include "SharedEnemyParticles.h"

static const char* navAgentProfileNames[] =
{
    "PlayerNormal",
    "PlayerSpectral",
    "EnemyGround"
};

constexpr int navAgentProfileCount = 3;

IMPLEMENT_SCRIPT_FIELDS(EnemyBaseController,
    SERIALIZED_ENUM_INT(m_enemyType, "Enemy Type", navAgentProfileNames, navAgentProfileCount),
    SERIALIZED_FLOAT(m_turnSpeedDegrees, "Turn Speed Degrees", 0.0f, 1080.0f, 1.0f),
    SERIALIZED_FLOAT(m_repathInterval, "Repath Interval", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(m_pathPointReachDistance, "Path Point Reach Distance", 0.01f, 5.0f, 0.01f),
    SERIALIZED_VEC3(m_pathSearchExtents, "Path Search Extents"),
    SERIALIZED_FLOAT(m_separationRadius, "Separation Radius", 0.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_separationStrength, "Separation Strength", 0.0f, 2.0f, 0.05f),
    SERIALIZED_FLOAT(m_separationQueryInterval, "Separation Query Interval", 0.0f, 1.0f, 0.01f)
)

EnemyBaseController::EnemyBaseController(GameObject* owner)
    : Script(owner)
{
}

void EnemyBaseController::Start()
{
    const EnemyBaseDataConfig* cfg = getBaseDataConfig();
    if (!cfg)
    {
        return;
    }

    m_moveSpeed = cfg->m_moveSpeed;
    m_recoveryDuration = cfg->m_recoveryDuration;
    m_stunnedDuration = cfg->m_stunnedDuration;
}

void EnemyBaseController::updateCurrentTarget()
{
    Transform* previousTarget = m_currentTarget;

    m_currentTarget = acquireCurrentTarget();

    if (m_currentTarget != previousTarget)
    {
        clearPath();
        resetRepathTimer();
    }
}

bool EnemyBaseController::hasValidTarget() const
{
    if (!m_currentTarget)
    {
        return false;
    }

    if (isTargetDowned(m_currentTarget))
    {
        return false;
    }

    return true;
}

const EnemyBaseDataConfig* EnemyBaseController::getBaseDataConfig() const
{
    return getAttackConfig();
}

float EnemyBaseController::getDistanceToCurrentTarget() const
{
    if (!m_currentTarget)
    {
        return FLT_MAX;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (!ownerTransform)
    {
        return FLT_MAX;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 targetPosition = TransformAPI::getGlobalPosition(m_currentTarget);

    Vector3 difference = targetPosition - ownerPosition;
    difference.y = 0.0f;

    return difference.Length();
}

bool EnemyBaseController::isCurrentTargetInRange(float range) const
{
    if (!hasValidTarget())
    {
        return false;
    }

    return getDistanceToCurrentTarget() <= range;
}

bool EnemyBaseController::isTargetInAttackRange() const
{
    if (!hasValidTarget())
    {
        return false;
    }

    const EnemyBaseAttackConfig* cfg = getAttackConfig();
    if (!cfg)
    {
        return false;
    }

    return isCurrentTargetInRange(cfg->m_basicAttackRange);
}

void EnemyBaseController::faceCurrentTarget()
{
    if (!m_currentTarget)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (!ownerTransform)
    {
        return;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 targetPosition = TransformAPI::getGlobalPosition(m_currentTarget);

    Vector3 direction = targetPosition - ownerPosition;
    direction.y = 0.0f;

    if (direction.LengthSquared() <= 0.00001f)
    {
        return;
    }

    rotateTowardsDirection(direction);
}

void EnemyBaseController::facePosition(const Vector3& worldPosition)
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (!ownerTransform)
    {
        return;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);

    Vector3 direction = worldPosition - ownerPosition;
    direction.y = 0.0f;

    if (direction.LengthSquared() <= 0.00001f)
    {
        return;
    }

    rotateTowardsDirection(direction);
}

bool EnemyBaseController::moveTowardsTarget()
{
    if (m_isForcedMovementActive)
    {
        return false;
    }

    if (!hasValidTarget())
    {
        clearPath();
        return false;
    }

    m_repathTimer += Time::getDeltaTime();

    if (!m_hasPath || m_repathTimer >= m_repathInterval)
    {
        if (!buildPathToTarget())
        {
            return false;
        }

        resetRepathTimer();
    }

    const bool moved = followPath();

    if (moved)
    {
        if (!m_enemySoundResolved)
        {
            m_enemySound = GameObjectAPI::findScript<EnemySound>(getOwner());
            m_enemySoundResolved = true;
        }

        if (m_enemySound)
        {
            m_enemySound->notifyMoving();
        }

        if (!m_sharedEnemyParticlesResolved)
        {
            m_sharedEnemyParticles = GameObjectAPI::findScript<SharedEnemyParticles>(getOwner());
            m_sharedEnemyParticlesResolved = true;
        }

        if (m_sharedEnemyParticles)
        {
            m_sharedEnemyParticles->notifyMoving();
        }
    }

    return moved;
}

void EnemyBaseController::clearPath()
{
    m_path.clear();
    m_currentPathIndex = 0;
    m_hasPath = false;

    m_noProgressTime = 0.0f;
}

void EnemyBaseController::resetRepathTimer()
{
    m_repathTimer = 0.0f;
}

void EnemyBaseController::setForcedMovementActive(bool active)
{
    if (m_isForcedMovementActive == active)
    {
        return;
    }

    m_isForcedMovementActive = active;

    clearPath();
    resetRepathTimer();
}

void EnemyBaseController::setForcedMovementBlocked(bool blocked)
{
    m_isForcedMovementBlocked = blocked;
}

bool EnemyBaseController::isDead() const
{
    Damageable* damageable = GameObjectAPI::findScript<Damageable>(getOwner());

    if (!damageable)
    {
        return false;
    }

    return damageable->isDead();
}

bool EnemyBaseController::trySendDeathTrigger(AnimationComponent* animation)
{
    if (m_deathTriggerSent)
    {
        return false;
    }

    if (!isDead())
    {
        return false;
    }

    if (!animation)
    {
        return false;
    }

    clearPath();

    const bool sent = AnimationAPI::sendTrigger(animation, "ToDeath");

    if (!sent)
    {
        return false;
    }

    m_deathTriggerSent = true;

    Debug::log("[EnemyBaseController] ToDeath trigger sent.");

    return true;
}

void EnemyBaseController::setRecoveryDuration(float recoveryDuration)
{
    m_recoveryDuration = recoveryDuration;
}

void EnemyBaseController::setStunnedDuration(float stunnedDuration)
{
    m_stunnedDuration = stunnedDuration;
}

void EnemyBaseController::useStun(float duration)
{
    if (duration <= 0.0f)
    {
        return;
    }

    m_isStunned = true;
    m_stunnedTriggerSent = false;
    m_stunnedTimer = duration;
    clearPath();
}

bool EnemyBaseController::trySendStunTrigger(AnimationComponent* animation)
{
    if (m_stunnedTriggerSent)
    {
        return false;
    }

    if (!m_isStunned)
    {
        return false;
    }

    if (isDead())
    {
        return false;
    }

    if (!animation)
    {
        return false;
    }

    clearPath();

    const bool sent = AnimationAPI::sendTrigger(animation, "ToStun");

    if (!sent)
    {
        return false;
    }

    m_stunnedTriggerSent = true;

    Debug::log("[EnemyBaseController] ToStun trigger sent.");

    return true;
}

void EnemyBaseController::clearStun()
{
    m_isStunned = false;
    m_stunnedTriggerSent = false;
}

void EnemyBaseController::updateStun(float dt)
{
    if (!m_isStunned)
    {
        return;
    }

    m_stunnedTimer -= dt;

    if (m_stunnedTimer <= 0.0f)
    {
        clearStun();
    }
}

void EnemyBaseController::rotateTowardsDirection(const Vector3& direction)
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (!ownerTransform)
    {
        return;
    }

    Vector3 desiredDirection = direction;
    desiredDirection.y = 0.0f;

    if (desiredDirection.LengthSquared() <= 0.00001f)
    {
        return;
    }

    desiredDirection.Normalize();

    Vector3 currentEuler = TransformAPI::getGlobalEulerDegrees(ownerTransform);

    constexpr float radiansToDegrees = 180.0f / 3.14159265f;

    const float desiredYawRadians = std::atan2(desiredDirection.x, desiredDirection.z);
    const float desiredYawDegrees = desiredYawRadians * radiansToDegrees;

    float deltaYaw = desiredYawDegrees - currentEuler.y;

    while (deltaYaw > 180.0f)
    {
        deltaYaw -= 360.0f;
    }

    while (deltaYaw < -180.0f)
    {
        deltaYaw += 360.0f;
    }

    const float maxStep = m_turnSpeedDegrees * Time::getDeltaTime();

    if (deltaYaw > maxStep)
    {
        deltaYaw = maxStep;
    }

    if (deltaYaw < -maxStep)
    {
        deltaYaw = -maxStep;
    }

    currentEuler.y += deltaYaw;

    TransformAPI::setGlobalRotationEuler(ownerTransform, currentEuler);
}

bool EnemyBaseController::buildPathToTarget()
{
    if (!hasValidTarget())
    {
        return false;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (!ownerTransform)
    {
        return false;
    }

    const Vector3 start = TransformAPI::getGlobalPosition(ownerTransform);
    const Vector3 destination = getPathDestination();

    Vector3 pathPoints[MAX_PATH_POINTS];

    const int pointCount = NavigationAPI::findStraightPath(start, destination, pathPoints, MAX_PATH_POINTS, m_pathSearchExtents, static_cast<NavAgentProfile>(m_enemyType));

    if (pointCount < 2)
    {
        clearPath();
        return false;
    }

    m_path = std::vector<Vector3>(pathPoints, pathPoints + pointCount);
    m_currentPathIndex = 1;
    m_hasPath = true;

    m_lastProgressPosition = start;
    m_noProgressTime = 0.0f;

    return true;
}

bool EnemyBaseController::followPath()
{
    if (!m_hasPath)
    {
        return false;
    }

    if (m_currentPathIndex >= m_path.size())
    {
        clearPath();
        return false;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (!ownerTransform)
    {
        return false;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 currentPathPoint = m_path[m_currentPathIndex];

    Vector3 toPoint = currentPathPoint - ownerPosition;
    toPoint.y = 0.0f;

    float distanceToPoint = toPoint.Length();

    if (distanceToPoint <= m_pathPointReachDistance)
    {
        ++m_currentPathIndex;

        if (m_currentPathIndex >= m_path.size())
        {
            clearPath();
            return false;
        }

        currentPathPoint = m_path[m_currentPathIndex];
        toPoint = currentPathPoint - ownerPosition;
        toPoint.y = 0.0f;
        distanceToPoint = toPoint.Length();

        if (distanceToPoint <= 0.0001f)
        {
            return false;
        }
    }

    toPoint.Normalize();

    Vector3 moveDirection = toPoint;

    const Vector3 separation = computeSeparationOffset(ownerPosition);

    if (separation.LengthSquared() > 0.0001f)
    {
        Vector3 blended = toPoint + separation * m_separationStrength;
        blended.y = 0.0f;

        if (blended.LengthSquared() > 0.0001f)
        {
            blended.Normalize();
            moveDirection = blended;
        }
    }

    const float maxStep = m_moveSpeed * Time::getDeltaTime();
    const Vector3 desiredStepTarget = ownerPosition + moveDirection * maxStep;

    Vector3 nextPosition;
    if (!NavigationAPI::moveAlongSurface(ownerPosition, desiredStepTarget, nextPosition, m_pathSearchExtents))
    {
        clearPath();
        return false;
    }

    Vector3 actualStep = nextPosition - ownerPosition;
    actualStep.y = 0.0f;

    facePosition(currentPathPoint);

    TransformAPI::setGlobalPosition(ownerTransform, nextPosition);

    // Progress check
    Vector3 progress = nextPosition - m_lastProgressPosition;
    progress.y = 0.0f;

    const float requiredProgressSquared = m_progressCheckDistance * m_progressCheckDistance;

    if (progress.LengthSquared() >= requiredProgressSquared)
    {
        m_lastProgressPosition = nextPosition;
        m_noProgressTime = 0.0f;
    }
    else
    {
        m_noProgressTime += Time::getDeltaTime();

        if (m_noProgressTime >= m_maxNoProgressTime)
        {
            clearPath();
            return false;
        }
    }

    return true;
}

Vector3 EnemyBaseController::getPathDestination() const
{
    if (!m_currentTarget)
    {
        Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

        if (!ownerTransform)
        {
            return Vector3::Zero;
        }

        return TransformAPI::getGlobalPosition(ownerTransform);
    }

    return TransformAPI::getGlobalPosition(m_currentTarget);
}

void EnemyBaseController::refreshNeighborCache()
{
    m_separationQueryTimer += Time::getDeltaTime();

    if (m_separationQueryTimer < m_separationQueryInterval)
    {
        return;
    }

    m_separationQueryTimer = 0.0f;

    m_neighborCache.clear();

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);

    for (GameObject* enemy : enemies)
    {
        if (enemy == nullptr)
        {
            continue;
        }

        Transform* enemyTransform = GameObjectAPI::getTransform(enemy);

        if (enemyTransform == nullptr || enemyTransform == ownerTransform)
        {
            continue;
        }

        Damageable* damageable = GameObjectAPI::findScript<Damageable>(enemy);

        if (damageable != nullptr && damageable->isDead())
        {
            continue;
        }

        m_neighborCache.push_back(enemyTransform);
    }
}

Vector3 EnemyBaseController::computeSeparationOffset(const Vector3& ownerPosition)
{
    if (!isSeparationEnabled() || m_separationRadius <= 0.0f || m_separationStrength <= 0.0f)
    {
        return Vector3::Zero;
    }

    refreshNeighborCache();

    const float radiusSq = m_separationRadius * m_separationRadius;

    Vector3 separation = Vector3::Zero;

    for (Transform* neighbor : m_neighborCache)
    {
        if (neighbor == nullptr)
        {
            continue;
        }

        Vector3 away = ownerPosition - TransformAPI::getGlobalPosition(neighbor);
        away.y = 0.0f;

        const float distSq = away.LengthSquared();

        if (distSq > radiusSq || distSq <= 0.0001f)
        {
            continue;
        }

        const float dist = std::sqrt(distSq);
        const float weight = 1.0f - (dist / m_separationRadius);

        away /= dist;

        separation += away * weight;
    }

    return separation;
}