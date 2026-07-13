#include "pch.h"
#include "DeathTaunt.h"
#include "DeathCharacter.h"
#include "DeathSound.h"
#include "EnemyDetectionAggro.h"
#include "PlayerRotation.h"
#include "PersistingPowerupState.h"
#include "EnemyShadowMark.h"
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

    if (m_character == nullptr || m_character->isDowned())
    {
        m_isAiming = false;
        m_currentAimDirection = Vector3::Zero;

        if (m_deathUI)
        {
            m_deathUI->hideTauntUI();
        }

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

void DeathTaunt::onAttackWindowFinished()
{
    if (m_movementLockedForCombo)
    {
        releaseComboMoveLock();
    }
}

float DeathTaunt::getCooldown() const
{
    const DeathConfig* cfg = m_config.get();
    if (!cfg) return 0.0f;
    return cfg->m_tauntCooldown;
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

    const DeathConfig* cfg = m_config.get();
    if (!cfg) return;

    Transform* ownerTransform = GameObjectAPI::getTransform(m_owner);
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);

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

    const float clampedHalfAngle = (cfg->m_tauntHalfAngleDegrees < 0.1f) ? 0.1f : ((cfg->m_tauntHalfAngleDegrees > 89.9f) ? 89.9f : cfg->m_tauntHalfAngleDegrees);
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
        Vector3 arcPoint = ownerPosition + direction * cfg->m_tauntRange;
        DebugDrawAPI::drawLine(ownerPosition, arcPoint, color, 0, false);
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
    Debug::log("[DeathTaunt] L2 released — casting.");
    m_isAiming = false;

    if (m_deathUI)
    {
        m_deathUI->hideTauntUI();
    }

    Vector3 finalDirection = m_currentAimDirection;
    if (!isAimStickValid(finalDirection))
    {
        finalDirection = getFallbackFacingDirection();
    }

    if (isAimStickValid(finalDirection))
    {
        faceDirection(finalDirection);

        DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
        if (sound != nullptr)
        {
            sound->playTauntShout();
        }

        if (m_deathParticles != nullptr)
        {
            m_deathParticles->SetTauntActive(finalDirection);
        }

        applyTauntToEnemiesInCone(finalDirection);
        notifyAbilitySuccessfullyStarted();
        m_debugConeTimer = 0.25f;

        const DeathConfig* cfg = m_config.get();
        m_movementLockedForCombo = true;
        beginAttackPresentation();
        beginAttackWindow(cfg ? cfg->m_tauntLockDuration : 0.0f);
    }

    m_currentAimDirection = Vector3::Zero;
    startCooldown();

    setAbilityLocked(false);
}

void DeathTaunt::applyTauntToEnemiesInCone(const Vector3& ownerForward) const
{
    const DeathConfig* cfg = m_config.get();
    if (!cfg) return;

    Transform* ownerTransform = GameObjectAPI::getTransform(m_owner);
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY);
    Debug::log("[DeathTaunt] Enemies with Tag::ENEMY found: %d", (int)enemies.size());

    int taunted = 0;
    bool anyMark = false;
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

        enemyAggro->applyTaunt(ownerTransform, cfg->m_tauntDuration);
        Debug::log("[DeathTaunt] Taunt applied to '%s' for %.1fs.", GameObjectAPI::getName(enemy), cfg->m_tauntDuration);

        if (PersistingPowerupState::isUnlocked(PowerupId::DeathPowerup1))
        {
            EnemyShadowMark* shadowMark = GameObjectAPI::findScript<EnemyShadowMark>(enemy);
            if (shadowMark != nullptr)
            {
                shadowMark->notifyDeathHit();
                anyMark = true;
            }
        }

        ++taunted;
    }

    if (anyMark)
    {
        DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
        if (sound != nullptr)
        {
            sound->playMarkApply();
        }
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
    const DeathConfig* cfg = m_config.get();
    if (!cfg) return false;

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
    if (distanceToEnemy <= 0.0f || distanceToEnemy > cfg->m_tauntRange)
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

    const float halfAngleRadians = cfg->m_tauntHalfAngleDegrees * (3.14159265f / 180.0f);
    const float coneThreshold = std::cos(halfAngleRadians);

    // TODO: Add a line-of-sight / wall check before confirming the taunt hit.
    return flattenedForward.Dot(directionToEnemy) >= coneThreshold;
}

IMPLEMENT_SCRIPT_FIELDS_INHERITED(DeathTaunt, DeathAbilityBase,
    SERIALIZED_ASSET_REF(m_config, "Death Config", AssetType::DATA_CONTAINER)
)

IMPLEMENT_SCRIPT(DeathTaunt)
