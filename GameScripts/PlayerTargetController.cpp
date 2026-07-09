#include "pch.h"
#include "PlayerTargetController.h"

#include "PlayerController.h"
#include "CharacterBase.h"
#include "Damageable.h"
#include "DeathSound.h"
#include "LyrielSound.h"
#include "EnemyDamageable.h"
#include "BreakableDamageable.h"

IMPLEMENT_SCRIPT_FIELDS(PlayerTargetController,
    SERIALIZED_FLOAT(m_targetRange, "Target Range", 0.0f, 20.0f, 0.05f),
    SERIALIZED_FLOAT(m_targetConeAngle, "Target Cone Angle", 1.0f, 180.0f, 1.0f),
    SERIALIZED_FLOAT(m_angleWeight, "Angle Weight", 0.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_distanceWeight, "Distance Weight", 0.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_switchMargin, "Switch Margin", 0.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_switchCooldown, "Switch Cooldown", 0.0f, 1.0f, 0.01f)
)

PlayerTargetController::PlayerTargetController(GameObject* owner)
    : Script(owner)
{
}

void PlayerTargetController::Start()
{
    m_character = GameObjectAPI::findScript<CharacterBase>(getOwner());
    m_playerController = GameObjectAPI::findScript<PlayerController>(getOwner());

    if (m_character == nullptr)
    {
        Debug::warn("PlayerTargetController on '%s' could not find CharacterBase-derived script on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }

    if (m_playerController == nullptr)
    {
        Debug::warn("PlayerTargetController on '%s' could not find PlayerController on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }

    m_deathSound  = GameObjectAPI::findScript<DeathSound>(getOwner());
    m_lyrielSound = GameObjectAPI::findScript<LyrielSound>(getOwner());
}

void PlayerTargetController::Update()
{
    if (m_playerController != nullptr && m_playerController->isGameplayInputLocked())
    {
        return;
    }
    
    const float dt = Time::getDeltaTime();

    if (m_switchCooldownTimer > 0.0f)
    {
        m_switchCooldownTimer -= dt;
    }

    updateTargetsInRange();
    ensureValidCurrentTarget();

    updateCurrentTarget();
}

void PlayerTargetController::drawGizmo()
{
    using namespace DebugDrawAPI;

    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);

    const Vector3 green = { 0.0f, 1.0f, 0.0f };
    const Vector3 yellow = { 1.0f, 1.0f, 0.0f };

    drawCircle(ownerPosition, Vector3(0.0f, 1.0f, 0.0f), green, m_targetRange, 32.0f, 0, true);

    if (m_currentTarget != nullptr)
    {
        Transform* targetTransform = GameObjectAPI::getTransform(m_currentTarget);
        if (targetTransform != nullptr)
        {
            const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);
            drawLine(ownerPosition, targetPosition, yellow, 0, true);
        }
    }
    
	const Vector3 aimDir = computeAimDirection();

    const Vector3 aimPosition = ownerPosition + aimDir * 3.0f;
    drawLine(ownerPosition, aimPosition, yellow, 0, true);

    const Vector3 posFlat = { ownerPosition.x, ownerPosition.y, ownerPosition.z };
    const float halfRad = m_targetConeAngle * 0.5f * (3.14159265f / 180.0f);
    const float range = m_targetRange;
    const Vector3 colBase = yellow;

    auto radialDir = [&](float a) -> Vector3
        {
            return Vector3(
                aimDir.x * cosf(a) + aimDir.z * sinf(a),
                0.0f,
                -aimDir.x * sinf(a) + aimDir.z * cosf(a));
        };

    // Arc outline
    DebugDrawAPI::drawLine(posFlat, posFlat + radialDir(-halfRad) * range, colBase);
    DebugDrawAPI::drawLine(posFlat, posFlat + radialDir(halfRad) * range, colBase);

}

void PlayerTargetController::updateTargetsInRange()
{
    m_targetsInRange.clear();

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);
    const std::vector<GameObject*> breakables = SceneAPI::findAllGameObjectsByTag(Tag::BREAKABLE, true);

    for (GameObject* enemy : enemies)
    {
        if (enemy == nullptr)
        {
            continue;
        }

        const bool inRange = isTargetInRange(enemy);

        if (inRange && isTargetAlive(enemy))
        {
            m_targetsInRange.push_back(enemy);
        }
    }

    for (GameObject* breakable : breakables)
    {
        if (breakable == nullptr)
        {
            continue;
        }

        const bool inRange = isTargetInRange(breakable);

        if (inRange && isTargetAlive(breakable))
        {
            m_targetsInRange.push_back(breakable);
        }
    }
}

void PlayerTargetController::updateCurrentTarget()
{
    if (!canUpdateTarget())
    {
        return;
    }

    const Vector3 aimDirection = computeAimDirection();

    if (!isAimStickValid(aimDirection))
    {
        return;
    }

    float bestScore = FLT_MAX;
    GameObject* bestTarget = findBestTarget(aimDirection, bestScore);

    if (bestTarget == nullptr)
    {
        return;
    }

    if (shouldSwitchTarget(bestTarget, aimDirection, bestScore))
    {
        setCurrentTarget(bestTarget);
    }
}

void PlayerTargetController::setCurrentTarget(GameObject* newTarget)
{
    if (m_currentTarget == newTarget)
    {
        return;
    }

    GameObject* previousTarget = m_currentTarget;
    m_currentTarget = newTarget;

    if (m_currentTarget != nullptr && previousTarget != nullptr)
    {
        m_switchCooldownTimer = m_switchCooldown;
    }

    if (m_currentTarget != nullptr)
    {
        const bool isLock = previousTarget == nullptr;

        if (m_deathSound != nullptr)
        {
            if (isLock)
            {
                m_deathSound->playLockTarget();
            }
            else
            {
                m_deathSound->playSwitchTarget();
            }
        }

        if (m_lyrielSound != nullptr)
        {
            if (isLock)
            {
                m_lyrielSound->playLockTarget();
            }
            else
            {
                m_lyrielSound->playSwitchTarget();
            }
        }

        Debug::log("[TargetSystem] Current target: %s", GameObjectAPI::getName(m_currentTarget));
    }
    else
    {
        Debug::log("[TargetSystem] No current target");
    }
}

void PlayerTargetController::ensureValidCurrentTarget()
{
    if (m_currentTarget == nullptr)
    {
        return;
    }

    if (!isTargetInRange(m_currentTarget) || !isTargetAlive(m_currentTarget))
    {
        setCurrentTarget(nullptr);
    }
}

bool PlayerTargetController::canUpdateTarget() const
{
    if (m_character == nullptr)
    {
        return false;
    }

    if (m_character->isDowned())
    {
        return false;
    }

    if (m_character->isUsingAbility())
    {
        return false;
    }

    return true;
}

Vector3 PlayerTargetController::computeAimDirection() const
{
    if (m_character == nullptr)
    {
        return Vector3::Zero;
    }

    const Vector2 lookAxis = Input::getLookAxis(m_character->getPlayerIndex());

    const float magSq = lookAxis.x * lookAxis.x + lookAxis.y * lookAxis.y;
    if (magSq <= 0.0001f)
    {
        return Vector3::Zero;
    }

    GameObject* cameraObject = SceneAPI::getDefaultCameraGameObject();
    if (cameraObject == nullptr)
    {
        Vector3 fallbackDirection(lookAxis.x, 0.0f, lookAxis.y);

        if (fallbackDirection.LengthSquared() > 0.0001f)
        {
            fallbackDirection.Normalize();
        }

        return fallbackDirection;
    }

    Transform* cameraTransform = GameObjectAPI::getTransform(cameraObject);
    if (cameraTransform == nullptr)
    {
        Vector3 fallbackDirection(lookAxis.x, 0.0f, lookAxis.y);

        if (fallbackDirection.LengthSquared() > 0.0001f)
        {
            fallbackDirection.Normalize();
        }

        return fallbackDirection;
    }

    Vector3 cameraForward = TransformAPI::getForward(cameraTransform);
    Vector3 cameraRight = TransformAPI::getRight(cameraTransform);

    cameraForward.y = 0.0f;
    cameraRight.y = 0.0f;

    if (cameraForward.LengthSquared() <= 0.0001f || cameraRight.LengthSquared() <= 0.0001f)
    {
        Vector3 fallbackDirection(lookAxis.x, 0.0f, lookAxis.y);

        if (fallbackDirection.LengthSquared() > 0.0001f)
        {
            fallbackDirection.Normalize();
        }

        return fallbackDirection;
    }

    cameraForward.Normalize();
    cameraRight.Normalize();

    Vector3 aimDirection = -cameraRight * lookAxis.x - cameraForward * lookAxis.y;

    if (aimDirection.LengthSquared() <= 0.0001f)
    {
        return Vector3::Zero;
    }

    aimDirection.Normalize();
    return aimDirection;
}

bool PlayerTargetController::isAimStickValid(const Vector3& direction) const
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    return flatDirection.LengthSquared() > 0.0001f;
}

bool PlayerTargetController::tryComputeTargetScore(GameObject* target, const Vector3& aimDirection, float& outScore) const
{
    outScore = FLT_MAX;

    if (target == nullptr)
    {
        return false;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    Transform* targetTransform = GameObjectAPI::getTransform(target);

    if (ownerTransform == nullptr || targetTransform == nullptr)
    {
        return false;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    ownerPosition.y = 0.0f;
    targetPosition.y = 0.0f;

    Vector3 toTarget = targetPosition - ownerPosition;

    // Reject out of range targets
    const float distanceSq = toTarget.LengthSquared();

    if (distanceSq <= 0.0001f)
    {
        return false;
    }

    const float targetRangeSq = m_targetRange * m_targetRange;

    if (distanceSq > targetRangeSq)
    {
        return false;
    }

    const float distance = sqrtf(distanceSq);

    // Normalize target and aim direction
    toTarget.Normalize();

    Vector3 flatAimDirection = aimDirection;
    flatAimDirection.y = 0.0f;

    if (flatAimDirection.LengthSquared() <= 0.0001f)
    {
        return false;
    }

    flatAimDirection.Normalize();

   // Check if target is inside the aim cone
   // Dot product tells how aligned the target is with the aim : 1.0 = perfectly aligned, 0.0 = perpendicular, -1.0 = behind.

    const float dot = flatAimDirection.Dot(toTarget);

    const float halfConeAngleRad = DirectX::XMConvertToRadians(m_targetConeAngle * 0.5f);
    const float minDot = cosf(halfConeAngleRad);

    if (dot < minDot)
    {
        return false;
    }

    // Here we compute target score, the lower the better result
    // angleScore, the closer to 0 the more aligned with the aim direction
    // distanceScore: the closer to 0 the closer to the player

    float angleScore = 1.0f - dot;

    float distanceScore = 0.0f;
    if (m_targetRange > 0.0001f)
    {
        distanceScore = distance / m_targetRange;
    }

    outScore = (angleScore * m_angleWeight) + (distanceScore * m_distanceWeight);

    return true;
}

GameObject* PlayerTargetController::findBestTarget(const Vector3& aimDirection, float& outBestScore) const
{
    GameObject* bestTarget = nullptr;
    outBestScore = FLT_MAX;

    // Check all currently valid targets and keep the one with the lowest score as the real target
    for (GameObject* target : m_targetsInRange)
    {
        float score = FLT_MAX;

        if (!tryComputeTargetScore(target, aimDirection, score))
        {
            continue;
        }

        if (score < outBestScore)
        {
            outBestScore = score;
            bestTarget = target;
        }
    }

    return bestTarget;
}

bool PlayerTargetController::shouldSwitchTarget(GameObject* candidate, const Vector3& aimDirection, float candidateScore) const
{
    if (candidate == nullptr)
    {
        return false;
    }

    if (m_currentTarget == nullptr)
    {
        return true;
    }

    // If the candidate is already the current target, no switch needed
    if (candidate == m_currentTarget)
    {
        return false;
    }

    if (m_switchCooldownTimer > 0.0f)
    {
        return false;
    }

    // If the current target can no longer be scored then replace it
    float currentScore = FLT_MAX;
    if (!tryComputeTargetScore(m_currentTarget, aimDirection, currentScore))
    {
        return true;
    }

    // Only switch if the new target is clearly better than the current one.
    return candidateScore + m_switchMargin < currentScore;
}

bool PlayerTargetController::isTargetInRange(GameObject* target) const
{
    if (target == nullptr)
    {
        return false;
    }

    GameObject* owner = getOwner();

    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    Transform* targetTransform = GameObjectAPI::getTransform(target);

    if (ownerTransform == nullptr || targetTransform == nullptr)
    {
        return false;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    const Vector3 distanceFromTarget = targetPosition - ownerPosition;
    const float distance = distanceFromTarget.Length();

    return distance <= m_targetRange;
}

bool PlayerTargetController::isTargetAlive(GameObject* target) const
{
    if (target == nullptr)
    {
        return false;
    }

    Script* damageableScript = GameObjectAPI::findScript<Damageable>(target);
    Damageable* damageable = dynamic_cast<Damageable*>(damageableScript);

    if (damageable != nullptr)
    {
        return !damageable->isDead() && damageable->getCurrentHp() > 0.0f;
    }

    return false;
}

IMPLEMENT_SCRIPT(PlayerTargetController)