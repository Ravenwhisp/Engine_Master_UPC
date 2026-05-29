#include "pch.h"
#include "RangedEnemyController.h"

#include "EnemyDetectionAggro.h"
#include "ArcherAttackConfig.h"

#include "Damageable.h"

#include <cfloat>
#include <cmath>
#include <algorithm>

IMPLEMENT_SCRIPT_FIELDS(RangedEnemyController,
    SERIALIZED_FLOAT(m_moveSpeed, "Move Speed", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_pathPointReachDistance, "Path Point Reach Distance", 0.01f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_repathInterval, "Repath Interval", 0.05f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_turnSpeedDegrees, "Turn Speed Degrees", 0.0f, 1080.0f, 1.0f)
)

RangedEnemyController::RangedEnemyController(GameObject* owner) : Script(owner)
{
}

void RangedEnemyController::Start()
{
    m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArcherAttackConfig>(getOwner());

    if (!m_enemyDetectionAggro)
    {
        Debug::warn("[RangedEnemyController] EnemyDetectionAggro not found on '%s'.", GameObjectAPI::getName(getOwner()));
    }

    if (!m_attackConfig)
    {
        Debug::warn("[RangedEnemyController] ArcherAttackConfig not found on '%s'.", GameObjectAPI::getName(getOwner()));
    }

    m_target = nullptr;
    m_repathTimer = 0.0f;
    m_lastTargetPosition = Vector3::Zero;
    m_deathTriggerSent = false;

    clearPath();
}

void RangedEnemyController::Update()
{
    const float dt = Time::getDeltaTime();

    updateCurrentTarget();
    updateSomersaultCooldown(dt);
    updateArrowBarrageCooldown(dt);

    m_repathTimer += dt;
}

void RangedEnemyController::updateCurrentTarget()
{
    if (!m_enemyDetectionAggro)
    {
        m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
    }

    Transform* previousTarget = m_target;

    if (!m_enemyDetectionAggro)
    {
        m_target = nullptr;
    }
    else
    {
        m_target = m_enemyDetectionAggro->getCurrentTarget();
    }

    if (m_target != previousTarget)
    {
        clearPath();
    }
}

float RangedEnemyController::getDistanceToTarget() const
{
    if (!m_target)
    {
        return FLT_MAX;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return FLT_MAX;
    }

    Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    Vector3 targetPosition = TransformAPI::getPosition(m_target);

    Vector3 delta = targetPosition - ownerPosition;
    delta.y = 0.0f;

    return delta.Length();
}

bool RangedEnemyController::isTargetInAttackRange() const
{
    if (!m_target || !m_attackConfig)
    {
        return false;
    }

    return getDistanceToTarget() <= m_attackConfig->m_basicAttackRange;
}

bool RangedEnemyController::moveTowardsTarget()
{
    if (!m_target)
    {
        clearPath();
        return false;
    }

    if (!NavigationAPI::hasNavMesh())
    {
        return false;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return false;
    }

    Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    Vector3 targetPosition = TransformAPI::getPosition(m_target);
    targetPosition.y = 0.0f;

    bool shouldRepath = !m_hasPath || m_repathTimer >= m_repathInterval;

    if (m_hasPath)
    {
        Vector3 targetDelta = targetPosition - m_lastTargetPosition;
        targetDelta.y = 0.0f;

        if (targetDelta.LengthSquared() > 1.0f)
        {
            shouldRepath = true;
        }
    }

    if (shouldRepath)
    {
        if (!rebuildPathToTarget())
        {
            return false;
        }

        m_repathTimer = 0.0f;
        m_lastTargetPosition = targetPosition;
    }

    if (!m_hasPath || m_currentPathIndex >= m_path.size())
    {
        return false;
    }

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

    const float maxStep = m_moveSpeed * Time::getDeltaTime();
    const Vector3 desiredStepTarget = ownerPosition + toPoint * maxStep;

    Vector3 nextPosition;
    if (!NavigationAPI::moveAlongSurface(ownerPosition, desiredStepTarget, nextPosition, Vector3(5.0f, 5.0f, 5.0f)))
    {
        clearPath();
        return false;
    }

    TransformAPI::setPosition(ownerTransform, nextPosition);

    Vector3 actualStep = nextPosition - ownerPosition;
    actualStep.y = 0.0f;

    if (actualStep.LengthSquared() <= 0.00001f)
    {
        clearPath();
        return false;
    }

    rotateTowardsDirection(actualStep);

    return true;
}

void RangedEnemyController::faceTarget()
{
    if (!m_target)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    Vector3 targetPosition = TransformAPI::getPosition(m_target);

    Vector3 direction = targetPosition - ownerPosition;
    direction.y = 0.0f;

    if (direction.LengthSquared() <= 0.00001f)
    {
        return;
    }

    rotateTowardsDirection(direction);
}

bool RangedEnemyController::rebuildPathToTarget()
{
    if (!m_target)
    {
        return false;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return false;
    }

    const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    const Vector3 targetPosition = TransformAPI::getPosition(m_target);

    constexpr int MAX_PATH_POINTS = 128;
    Vector3 pathPoints[MAX_PATH_POINTS];

    const int pointCount = NavigationAPI::findStraightPath(
        ownerPosition,
        targetPosition,
        pathPoints,
        MAX_PATH_POINTS,
        Vector3(5.0f, 5.0f, 5.0f)
    );

    if (pointCount < 2)
    {
        return false;
    }

    m_path = std::vector<Vector3>(pathPoints, pathPoints + pointCount);
    m_currentPathIndex = 1;
    m_hasPath = true;

    return true;
}

void RangedEnemyController::clearPath()
{
    m_path.clear();
    m_currentPathIndex = 0;
    m_hasPath = false;
}

bool RangedEnemyController::isDead() const
{
    Damageable* damageable = GameObjectAPI::findScript<Damageable>(getOwner());

    if (damageable && damageable->isDead())
    {
        return true;
    }

    return false;
}

bool RangedEnemyController::trySendDeathTrigger(AnimationComponent* animation)
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

    Debug::log("[RangedEnemyController] ToDeath trigger sent.");

    return true;
}

void RangedEnemyController::rotateTowardsDirection(const Vector3& direction)
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 moveDir = direction;
    moveDir.y = 0.0f;

    if (moveDir.LengthSquared() <= 0.00001f)
    {
        return;
    }

    moveDir.Normalize();

    Vector3 currentEuler = TransformAPI::getEulerDegrees(ownerTransform);

    const float desiredYawRadians = std::atan2(moveDir.x, moveDir.z);
    float desiredYawDegrees = DirectX::XMConvertToDegrees(desiredYawRadians);

    float currentYawDegrees = currentEuler.y;
    float deltaYaw = desiredYawDegrees - currentYawDegrees;

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
    TransformAPI::setRotationEuler(ownerTransform, currentEuler);
}

bool RangedEnemyController::playerInSomersaultRange() const
{
    if (!m_enemyDetectionAggro || !m_attackConfig)
    {
        return false;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return false;
    }

    Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);

    Transform* playerTransforms[] =
    {
        m_enemyDetectionAggro->getLyrielTransform(),
        m_enemyDetectionAggro->getDeathTransform()
    };

    const float triggerRangeSquared = m_attackConfig->m_somersaultTriggerRange * m_attackConfig->m_somersaultTriggerRange;

    for (Transform* playerTransform : playerTransforms)
    {
        if (!playerTransform)
        {
            continue;
        }

        if (m_enemyDetectionAggro->isDowned(playerTransform))
        {
            continue;
        }

        Vector3 playerPosition = TransformAPI::getPosition(playerTransform);

        Vector3 difference = playerPosition - ownerPosition;
        difference.y = 0.0f;

        if (difference.LengthSquared() <= triggerRangeSquared)
        {
            return true;
        }
    }

    return false;
}

bool RangedEnemyController::isSomersaultReady() const
{
    return m_somersaultCooldownTimer <= 0.0f;
}

void RangedEnemyController::consumeSomersaultCooldown()
{
    if (!m_attackConfig)
    {
        return;
    }

    m_somersaultCooldownTimer = m_attackConfig->m_somersaultCooldown;
}

void RangedEnemyController::updateSomersaultCooldown(float dt)
{
    if (m_somersaultCooldownTimer <= 0.0f)
    {
        return;
    }

    m_somersaultCooldownTimer -= dt;

    if (m_somersaultCooldownTimer < 0.0f)
    {
        m_somersaultCooldownTimer = 0.0f;
    }
}

Vector3 RangedEnemyController::getDirectionAwayFromClosestPlayer() const
{
    if (!m_enemyDetectionAggro)
    {
        return Vector3(0.0f, 0.0f, -1.0f);
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);

    Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
    Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

    Transform* closestTransform = nullptr;
    float closestDistanceSquared = FLT_MAX;

    Transform* playerTransforms[] =
    {
        lyrielTransform,
        deathTransform
    };

    for (Transform* playerTransform : playerTransforms)
    {
        if (m_enemyDetectionAggro->isDowned(playerTransform))
        {
            continue;
        }

        Vector3 playerPosition = TransformAPI::getPosition(playerTransform);
        Vector3 difference = ownerPosition - playerPosition;
        difference.y = 0.0f;

        const float distanceSquared = difference.LengthSquared();

        if (distanceSquared < closestDistanceSquared)
        {
            closestDistanceSquared = distanceSquared;
            closestTransform = playerTransform;
        }
    }

    if (!closestTransform)
    {
        Vector3 backward = TransformAPI::getForward(ownerTransform) * -1.0f;
        backward.y = 0.0f;

        if (backward.LengthSquared() <= 0.00001f)
        {
            return Vector3(0.0f, 0.0f, -1.0f);
        }

        backward.Normalize();
        return backward;
    }

    Vector3 closestPosition = TransformAPI::getPosition(closestTransform);
    Vector3 escapeDirection = ownerPosition - closestPosition;
    escapeDirection.y = 0.0f;

    if (escapeDirection.LengthSquared() <= 0.00001f)
    {
        Vector3 backward = TransformAPI::getForward(ownerTransform) * -1.0f;
        backward.y = 0.0f;

        if (backward.LengthSquared() <= 0.00001f)
        {
            return Vector3(0.0f, 0.0f, -1.0f);
        }

        backward.Normalize();
        return backward;
    }

    escapeDirection.Normalize();
    return escapeDirection;
}

bool RangedEnemyController::isArrowBarrageReady() const
{
    return m_arrowBarrageCooldownTimer <= 0.0f;
}

void RangedEnemyController::consumeArrowBarrageCooldown()
{
    if (!m_attackConfig)
    {
        return;
    }

    m_arrowBarrageCooldownTimer = m_attackConfig->m_arrowBarrageCooldown;
}

void RangedEnemyController::updateArrowBarrageCooldown(float dt)
{
    if (m_arrowBarrageCooldownTimer <= 0.0f)
    {
        return;
    }

    m_arrowBarrageCooldownTimer -= dt;

    if (m_arrowBarrageCooldownTimer < 0.0f)
    {
        m_arrowBarrageCooldownTimer = 0.0f;
    }
}

bool RangedEnemyController::isTargetInArrowBarrageRange() const
{
    if (!m_target || !m_attackConfig)
    {
        return false;
    }

    return getDistanceToTarget() <= m_attackConfig->m_arrowBarrageRange;
}

IMPLEMENT_SCRIPT(RangedEnemyController)