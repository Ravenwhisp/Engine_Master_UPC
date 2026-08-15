#include "pch.h"
#include "DeathTaunt.h"
#include "DeathCharacter.h"
#include "DeathSound.h"
#include "EnemyDetectionAggro.h"
#include "EnemyDamageable.h"
#include "EnemyForcedMovement.h"
#include "PlayerRotation.h"
#include "DeathUI.h"
#include "DeathConfig.h"
#include "DeathParticles.h"

#include <cmath>

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

    m_deathUI = GameObjectAPI::findScript<DeathUI>(getOwner());

    if (!m_deathUI)
    {
        Debug::warn("[DeathTaunt] DeathUI not found.");
    }

    m_deathParticles = GameObjectAPI::findScript<DeathParticles>(getOwner());

    if (!m_deathParticles)
    {
        Debug::warn("[DeathTaunt] DeathParticles not found.");
    }
}

void DeathTaunt::Update()
{
    DeathAbilityBase::Update();

    if (m_tauntState == TauntState::WaitingForImpact)
    {
        updateImpactDelay();
    }

    if (m_character == nullptr || m_character->isDowned())
    {
        if (m_tauntState == TauntState::Aiming)
        {
            m_tauntState = TauntState::Idle;
        }

        m_currentAimDirection = Vector3::Zero;

        if (m_deathUI)
        {
            m_deathUI->hideTauntUI();
        }

        return;
    }

    if (m_tauntState == TauntState::Aiming)
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

    if (m_tauntState != TauntState::Aiming && m_debugConeTimer > 0.0f)
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
    return m_tauntState == TauntState::Idle;
}

void DeathTaunt::onAttackWindowFinished()
{
    if (m_tauntState == TauntState::WaitingForImpact)
    {
        return;
    }

    if (m_movementLockedForCombo)
    {
        releaseComboMoveLock();
    }
}

float DeathTaunt::getCooldown() const
{
    return m_deathCharacter->getConfig()->m_tauntCooldown;
}

void DeathTaunt::startAbility()
{
	beginAim();
}

void DeathTaunt::drawGizmo()
{
    if (m_tauntState != TauntState::Aiming && m_debugConeTimer <= 0.0f)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(m_owner);
    if (ownerTransform == nullptr)
    {
        return;
    }

    Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 ownerForward = m_currentAimDirection;

    if (m_tauntState == TauntState::WaitingForImpact)
    {
        ownerPosition = m_castOrigin;
        ownerForward = m_castDirection;
    }
    else if (ownerForward.LengthSquared() <= 0.0001f)
    {
        ownerForward = getFallbackFacingDirection();
    }

    if (ownerForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    ownerForward.Normalize();

    const float clampedHalfAngle = (m_deathCharacter->getConfig()->m_tauntHalfAngleDegrees < 0.1f) ? 0.1f : ((m_deathCharacter->getConfig()->m_tauntHalfAngleDegrees > 89.9f) ? 89.9f : m_deathCharacter->getConfig()->m_tauntHalfAngleDegrees);
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
        Vector3 arcPoint = ownerPosition + direction * m_deathCharacter->getConfig()->m_tauntRange;
        DebugDrawAPI::drawLine(ownerPosition, arcPoint, color, 0, false);
    }
}

void DeathTaunt::beginAim()
{
    Debug::log("[DeathTaunt] Aim started.");
    m_tauntState = TauntState::Aiming;
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

    if (m_deathUI)
    {
        m_deathUI->showTauntUI();
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
    
    if (m_deathUI)
    {
        const Vector3 origin = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner()));
        m_deathUI->updateTauntUI(origin, m_currentAimDirection);
    }
}

void DeathTaunt::releaseAimAndCast()
{
    if (m_deathUI)
    {
        m_deathUI->hideTauntUI();
    }

    Vector3 finalDirection = m_currentAimDirection;
    if (!isAimStickValid(finalDirection))
    {
        finalDirection = getFallbackFacingDirection();
        m_tauntState = TauntState::Idle;
        m_currentAimDirection = Vector3::Zero;
        setAbilityLocked(false);
        return;
    }

    finalDirection.y = 0.0f;
    finalDirection.Normalize();

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    faceDirection(finalDirection);

    m_castOrigin = TransformAPI::getGlobalPosition(ownerTransform);
    m_castDirection = finalDirection;
    m_impactDelayTimer = m_deathCharacter->getConfig()->m_tauntImpactDelay;
    m_tauntState = TauntState::WaitingForImpact;

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->playTauntShout();
    }

    if (m_deathParticles != nullptr)
    {
        m_deathParticles->SetTauntActive(finalDirection);
    }

    notifyAbilitySuccessfullyStarted();
    m_debugConeTimer = m_deathCharacter->getConfig()->m_tauntImpactDelay;

    m_movementLockedForCombo = true;
    beginAttackPresentation();
    beginAttackWindow(m_deathCharacter->getConfig()->m_tauntImpactDelay);

    m_currentAimDirection = Vector3::Zero;

    startCooldown();
    setAbilityLocked(false);
}

void DeathTaunt::updateImpactDelay()
{
    m_impactDelayTimer -= Time::getDeltaTime();

    if (m_impactDelayTimer > 0.0f)
    {
        return;
    }

    m_impactDelayTimer = 0.0f;

    resolveImpact();
}

void DeathTaunt::resolveImpact()
{
    Transform* deathTransform = GameObjectAPI::getTransform(getOwner());

    const std::vector<GameObject*> enemies = collectEnemiesInCone(m_castOrigin, m_castDirection);

    int taunted = 0;
    int pulled = 0;

    for (GameObject* enemy : enemies)
    {
        applyTauntEffects(enemy, deathTransform);
        ++taunted;

        // This is an edge case where the enemy affected is closer to the destination distance. In this case it won't be affected by the pull
        Transform* enemyTransform = GameObjectAPI::getTransform(enemy);

        const Vector3 enemyPosition = TransformAPI::getGlobalPosition(enemyTransform);

        Vector3 offsetFromOrigin = enemyPosition - m_castOrigin;
        offsetFromOrigin.y = 0.0f;

        if (offsetFromOrigin.Length() <= m_deathCharacter->getConfig()->m_tauntPullDestinationDistance)
        {
            EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(enemy);

            if (damageable)
            {
                EnemyHitContext hitContext;
                hitContext.damage = m_deathCharacter->getConfig()->m_tauntPullDamage;
                hitContext.attacker = deathTransform;
                hitContext.attackType = PlayerAttackType::DeathTaunt;

                damageable->takeDamage(hitContext);
            }

            continue;
        }
        //

        EnemyForcedMovement* forcedMovement = GameObjectAPI::findScript<EnemyForcedMovement>(enemy);

        if (!forcedMovement)
        {
            Debug::warn("[DeathTaunt] Enemy '%s' has no EnemyForcedMovement. Taunt applied without pull.", GameObjectAPI::getName(enemy));
            continue;
        }

        const Vector3 destination = calculatePullDestination(enemy);

        const bool pullStarted = forcedMovement->startPull(destination, m_deathCharacter->getConfig()->m_tauntPullDuration, deathTransform, m_deathCharacter->getConfig()->m_tauntPullDamage, PlayerAttackType::DeathTaunt);

        if (pullStarted)
        {
            ++pulled;
        }
    }

    m_tauntState = TauntState::Idle;

    if (m_movementLockedForCombo)
    {
        releaseComboMoveLock();
    }

    Debug::log("[DeathTaunt] Impact resolved — %d taunted, %d pulled.", taunted, pulled);
}

std::vector<GameObject*> DeathTaunt::collectEnemiesInCone(const Vector3& origin, const Vector3& direction) const
{
    std::vector<GameObject*> validEnemies;

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY);

    for (GameObject* enemy : enemies)
    {
        if (!isEnemyInsideTauntCone(enemy, origin, direction))
        {
            continue;
        }

        EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(enemy);

        if (damageable && damageable->isDead())
        {
            continue;
        }

        validEnemies.push_back(enemy);
    }

    return validEnemies;
}

void DeathTaunt::applyTauntEffects(GameObject* enemy, Transform* deathTransform) const
{
    if (!enemy || !deathTransform)
    {
        return;
    }

    EnemyDetectionAggro* enemyAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(enemy);

    if (enemyAggro)
    {
        enemyAggro->applyTaunt(deathTransform, m_deathCharacter->getConfig()->m_tauntDuration);
    }
}

Vector3 DeathTaunt::calculatePullDestination(GameObject* enemy) const
{
    if (!enemy)
    {
        return m_castOrigin;
    }

    Transform* enemyTransform = GameObjectAPI::getTransform(enemy);

    const Vector3 enemyPosition = TransformAPI::getGlobalPosition(enemyTransform);

    Vector3 directionFromOrigin = enemyPosition - m_castOrigin;
    directionFromOrigin.y = 0.0f;

    if (directionFromOrigin.LengthSquared() <= 0.0001f)
    {
        directionFromOrigin = m_castDirection;
        directionFromOrigin.y = 0.0f;
        return m_castOrigin;
    }

    directionFromOrigin.Normalize();

    Vector3 finalDestination  = m_castOrigin + directionFromOrigin * m_deathCharacter->getConfig()->m_tauntPullDestinationDistance;

    return finalDestination;
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

    Vector3 directionToEnemy = TransformAPI::getGlobalPosition(enemyTransform) - ownerPosition;
    directionToEnemy.y = 0.0f;

    const float distanceToEnemy = directionToEnemy.Length();
    if (distanceToEnemy <= 0.0f || distanceToEnemy > m_deathCharacter->getConfig()->m_tauntRange)
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

    const float halfAngleRadians = m_deathCharacter->getConfig()->m_tauntHalfAngleDegrees * (3.14159265f / 180.0f);
    const float coneThreshold = std::cos(halfAngleRadians);

    // TODO: Add a line-of-sight / wall check before confirming the taunt hit.
    return flattenedForward.Dot(directionToEnemy) >= coneThreshold;
}

IMPLEMENT_SCRIPT(DeathTaunt)
