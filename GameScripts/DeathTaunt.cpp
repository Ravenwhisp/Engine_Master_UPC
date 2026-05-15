#include "pch.h"
#include "DeathTaunt.h"
#include "DeathCharacter.h"
#include "EnemyDetectionAggro.h"
#include "PlayerRotation.h"
#include "PersistingPowerupState.h"
#include "EnemyShadowMark.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(DeathTaunt, DeathAbilityBase,
    SERIALIZED_FLOAT(m_tauntDuration, "Taunt Duration", 0.0f, 10.0f, 0.1f),
    SERIALIZED_COMPONENT_REF(m_AbilityUI, "Ability UI", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_TauntDurationSeconds, "Ability Duration", 1.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_TauntRange, "Cone Range", 1.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_TauntHalfAngleDegrees, "Cone Angle", 1.0f, 180.0f, 1.0f)
)

DeathTaunt::DeathTaunt(GameObject* owner)
    : DeathAbilityBase(owner)
{
}

void DeathTaunt::Start()
{
    DeathAbilityBase::Start();

    if (m_character != nullptr)
    {
        m_playerRotation = m_character->getPlayerRotation();
    }
}

void DeathTaunt::Update()
{
    DeathAbilityBase::Update();

    if (m_character == nullptr || m_character->isDowned())
    {
        m_isAiming = false;
        m_currentAimDirection = Vector3::Zero;
        return;
    }

    if (m_isAiming)
    {
        if (Input::isLeftTriggerPressed(getPlayerIndex()))
        {
            updateAim();
        }

        if (Input::isLeftTriggerReleased(getPlayerIndex()))
        {
            releaseAimAndCast();
        }
    }

    if (!m_isAiming && m_debugConeTimer > 0.0f)
    {
        m_debugConeTimer -= Time::getDeltaTime();
        if (m_debugConeTimer < 0.0f)
        {
            m_debugConeTimer = 0.0f;
        }
    }
}

bool DeathTaunt::canStartSpecificAbility() const
{
    return !m_isAiming;
}

void DeathTaunt::startAbility()
{
	beginAim();
}

void DeathTaunt::drawGizmo()
{
    if (!m_isAiming && m_debugConeTimer <= 0.0f)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(m_owner);
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);

    Vector3 ownerForward = m_currentAimDirection;
    if (ownerForward.LengthSquared() <= 0.0001f)
    {
        ownerForward = getFallbackFacingDirection();
    }

    if (ownerForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    ownerForward.Normalize();

    const float clampedHalfAngle = (m_TauntHalfAngleDegrees < 0.1f) ? 0.1f : ((m_TauntHalfAngleDegrees > 89.9f) ? 89.9f : m_TauntHalfAngleDegrees);
    const float halfAngleRadians = clampedHalfAngle * (3.14159265f / 180.0f);

    const int numSteps = 16;
    const float angleStep = (2.0f * halfAngleRadians) / (numSteps - 1);
    const Vector3 color(1.0f, 0.0f, 0.0f);

    for (int i = 0; i < numSteps; ++i)
    {
        float angle = -halfAngleRadians + i * angleStep;
        Vector3 direction = ownerForward;
        direction = Vector3(
            direction.x * std::cos(angle) - direction.z * std::sin(angle),
            0.0f,
            direction.x * std::sin(angle) + direction.z * std::cos(angle)
        );
        direction.Normalize();
        Vector3 arcPoint = ownerPosition + direction * m_TauntRange;
        DebugDrawAPI::drawLine(ownerPosition, arcPoint, color, 0, false);
    }
}

void DeathTaunt::onFieldEdited(const ScriptFieldInfo& field)
{
    if (strcmp(field.name, "Cone Range") == 0 || strcmp(field.name, "Cone Angle") == 0)
    {
        m_debugConeTimer = 1.0f;
    }
}

void DeathTaunt::beginAim()
{
    Debug::log("[DeathTaunt] Aim started.");
    m_isAiming = true;
    setAbilityLocked(true);
    m_debugConeTimer = 0.25f;
    m_currentAimDirection = getFallbackFacingDirection();

    Vector3 aimDirection = computeAimDirection();
    if (isAimStickValid(aimDirection))
    {
        m_currentAimDirection = aimDirection;
    }
    else
    {
		m_currentAimDirection = getFallbackFacingDirection();
    }
    faceDirection(m_currentAimDirection);
    if (m_AbilityUI.getReferencedComponent())
    {
        GameObjectAPI::setActive(m_AbilityUI.getReferencedComponent()->getOwner(), true);
    }
}

void DeathTaunt::updateAim()
{
    Vector3 aimDirection = computeAimDirection();
    if (isAimStickValid(aimDirection))
    {
        m_currentAimDirection = aimDirection;
        faceDirection(m_currentAimDirection);
    }
    if (m_AbilityUI.getReferencedComponent())
    {
        const Vector3 origin = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner()));
        const float yawRad = std::atan2(m_currentAimDirection.x, m_currentAimDirection.z);
        const float targetYawDeg = yawRad * (180.0f / 3.14159265f);

        TransformAPI::setPosition(m_AbilityUI.getReferencedComponent(), origin);
        TransformAPI::setRotationEuler(m_AbilityUI.getReferencedComponent(), Vector3(0.0f, targetYawDeg, 0.0f));
    }
}

void DeathTaunt::releaseAimAndCast()
{
    Debug::log("[DeathTaunt] L2 released — casting.");
    m_isAiming = false;

    if (m_AbilityUI.getReferencedComponent())
    {
        GameObjectAPI::setActive(m_AbilityUI.getReferencedComponent()->getOwner(), false);
    }

    Vector3 finalDirection = m_currentAimDirection;
    if (!isAimStickValid(finalDirection))
    {
        finalDirection = getFallbackFacingDirection();
    }

    if (isAimStickValid(finalDirection))
    {
        faceDirection(finalDirection);
        applyTauntToEnemiesInCone(finalDirection);
        m_debugConeTimer = 0.25f;
    }

    m_currentAimDirection = Vector3::Zero;
    startCooldown();

    setAbilityLocked(false);
}

void DeathTaunt::applyTauntToEnemiesInCone(const Vector3& ownerForward) const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(m_owner);
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY);
    Debug::log("[DeathTaunt] Enemies with Tag::ENEMY found: %d", (int)enemies.size());

    int taunted = 0;
    for (GameObject* enemy : enemies)
    {
        if (!isEnemyInsideTauntCone(enemy, ownerPosition, ownerForward))
        {
            Debug::log("[DeathTaunt] Enemy '%s' outside cone.", GameObjectAPI::getName(enemy));
            continue;
        }

        EnemyDetectionAggro* enemyAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(enemy);
        if (enemyAggro == nullptr)
        {
            Debug::log("[DeathTaunt] Enemy '%s' in cone but no EnemyDetectionAggro.", GameObjectAPI::getName(enemy));
            continue;
        }

        enemyAggro->applyTaunt(ownerTransform, m_TauntDurationSeconds);
        Debug::log("[DeathTaunt] Taunt applied to '%s' for %.1fs.", GameObjectAPI::getName(enemy), m_TauntDurationSeconds);

        if (PersistingPowerupState::isUnlocked(PowerupId::DeathPowerup1))
        {
            EnemyShadowMark* shadowMark = GameObjectAPI::findScript<EnemyShadowMark>(enemy);
            if (shadowMark != nullptr)
            {
                shadowMark->notifyDeathHit();
            }
        }

        ++taunted;
    }

    Debug::log("[DeathTaunt] Cast complete — %d enemies taunted.", taunted);
}

Vector3 DeathTaunt::computeAimDirection() const
{
    return computeCameraRelativeAimDirection();
}

void DeathTaunt::faceDirection(const Vector3& direction)
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatDirection.Normalize();

    if (m_playerRotation != nullptr)
    {
        m_playerRotation->applyFacingFromDirection(getOwner(), flatDirection, Time::getDeltaTime());
    }
}

bool DeathTaunt::isAimStickValid(const Vector3& direction) const
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    return flatDirection.LengthSquared() > 0.0001f;
}

bool DeathTaunt::isEnemyInsideTauntCone(GameObject* enemy, const Vector3& ownerPosition, const Vector3& ownerForward) const
{
    if (enemy == nullptr)
    {
        return false;
    }

    Transform* enemyTransform = GameObjectAPI::getTransform(enemy);
    if (enemyTransform == nullptr)
    {
        return false;
    }

    Vector3 directionToEnemy = TransformAPI::getPosition(enemyTransform) - ownerPosition;
    directionToEnemy.y = 0.0f;

    const float distanceToEnemy = directionToEnemy.Length();
    if (distanceToEnemy <= 0.0f || distanceToEnemy > m_TauntRange)
    {
        return false;
    }

    if (ownerForward.LengthSquared() <= 0.0001f)
    {
        return false;
    }

    Vector3 flattenedForward = ownerForward;
    flattenedForward.Normalize();
    directionToEnemy.Normalize();

    const float halfAngleRadians = m_TauntHalfAngleDegrees * (3.14159265f / 180.0f);
    const float coneThreshold = std::cos(halfAngleRadians);

    // TODO: Add a line-of-sight / wall check before confirming the taunt hit.
    return flattenedForward.Dot(directionToEnemy) >= coneThreshold;
}

IMPLEMENT_SCRIPT(DeathTaunt)
