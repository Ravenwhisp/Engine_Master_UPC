#include "pch.h"
#include "LyrielArrowVolley.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "CharacterBase.h"
#include "ArrowPool.h"
#include "LyrielArrowProjectile.h"
#include "EnemyDamageable.h"
#include "PlayerState.h"
#include "PersistingPowerupState.h"
#include "EnemyShadowMark.h"
#include "BreakableDamageable.h"
#include "LyrielUI.h"
#include "LyrielConfig.h"

#include <cmath>

LyrielArrowVolley::LyrielArrowVolley(GameObject* owner)
    : LyrielAbilityBase(owner)
{
}

void LyrielArrowVolley::Start()
{
    LyrielAbilityBase::Start();

    m_lyrielUI = GameObjectAPI::findScript<LyrielUI>(getOwner());

    if (!m_lyrielUI)
    {
        Debug::warn("[LyrielArrowVolley] LyrielUI not found.");
    }
}

void LyrielArrowVolley::Update()
{
    LyrielAbilityBase::Update();

    if(m_isAiming)
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
}

void LyrielArrowVolley::startAbility()
{
    beginAim();
}

void LyrielArrowVolley::drawGizmo()
{
    if (!m_isAiming)
    {
        return;
    }

    Vector3 previewDirection = m_currentAimDirection;
    if (previewDirection.LengthSquared() <= 0.0001f)
    {
        previewDirection = getFallbackFacingDirection();
    }

    if (previewDirection.LengthSquared() <= 0.0001f)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);
    drawAimPreview(origin, previewDirection);
}

void LyrielArrowVolley::onAttackWindowUpdate()
{
    if (m_attackFacingDirection.LengthSquared() > 0.0001f)
    {
        faceDirection(m_attackFacingDirection);
    }
}

void LyrielArrowVolley::onAttackWindowFinished()
{
    m_attackFacingDirection = Vector3::Zero;
}

float LyrielArrowVolley::getCooldown() const
{
    return m_config->m_volleyCooldown;
}

bool LyrielArrowVolley::canCast() const
{
    return m_character != nullptr && !m_character->isDowned();
}

void LyrielArrowVolley::beginAim()
{
    m_isAiming = true;
    setAbilityLocked(true);

    m_currentAimDirection = Vector3::Zero;

    Vector3 aimDirection = computeAimDirection();
    if (isAimStickValid(aimDirection))
    {
        m_currentAimDirection = aimDirection;
    }
    else
    {
		m_currentAimDirection = getFallbackFacingDirection();
    }

    if (m_lyrielUI)
    {
        m_lyrielUI->showArrowVolleyUI();
    }
}

void LyrielArrowVolley::updateAim()
{
    Vector3 aimDirection = computeAimDirection();
    if (isAimStickValid(aimDirection))
    {
        m_currentAimDirection = aimDirection;
    }
    
    if (m_lyrielUI)
    {
        Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

        if (ownerTransform)
        {
            const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);
            m_lyrielUI->updateArrowVolleyUI(origin, m_currentAimDirection);
        }
    }
}

void LyrielArrowVolley::releaseAimAndCast()
{
    m_isAiming = false;

    if (m_lyrielUI)
    {
        m_lyrielUI->hideArrowVolleyUI();
    }

    if (!canCast())
    {
        setAbilityLocked(false);
        return;
    }

    Transform* spawnTransform = findArrowSpawnTransform();
    if (spawnTransform == nullptr)
    {
        setAbilityLocked(false);
        return;
    }

    const Vector3 origin = TransformAPI::getGlobalPosition(spawnTransform);

    Vector3 forward = m_currentAimDirection;
    if (forward.LengthSquared() <= 0.0001f)
    {
        forward = getFallbackFacingDirection();
    }

    if (forward.LengthSquared() <= 0.0001f)
    {
        setAbilityLocked(false);
        return;
    }

    m_attackFacingDirection = forward;
    faceDirection(forward);

    std::vector<Damageable*> targets;
    collectEnemiesInCone(origin, forward, targets);
    const bool anyMarkExploited = applyVolleyDamage(targets);
    spawnVolleyArrows(origin, forward);

    LyrielSound* sound = m_lyrielCharacter != nullptr ? m_lyrielCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->playVolleyRelease();
        if (anyMarkExploited)
        {
            sound->playMarkExploit();
        }
    }

    beginAttackPresentation();

    beginAttackWindow(m_config->m_volleyAttackLockDuration);
    startCooldown();

    Debug::log("[LyrielArrowVolley] Cast Arrow Volley. Targets hit: %d", static_cast<int>(targets.size()));
}

Vector3 LyrielArrowVolley::computeAimDirection() const
{
    return computeCameraRelativeAimDirection();
}

bool LyrielArrowVolley::isAimStickValid(const Vector3& direction) const
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;
    return flatDirection.LengthSquared() > 0.0001f;
}

void LyrielArrowVolley::collectEnemiesInCone(const Vector3& origin, const Vector3& forward, std::vector<Damageable*>& outTargets)
{
    outTargets.clear();

    //std::vector<GameObject*> allEnemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true); //cambiar, esto no pilla los damageables

    //detectar enemigos en cono

	const std::vector<GameObject*> objectsInCircularRange = SceneAPI::getObjectsInCircularArea(Vector2(origin.x, origin.z), m_config->m_volleyRange);

    std::vector<Damageable*> damageables;

    for(GameObject* obj : objectsInCircularRange)
    {
        if (obj->GetTag() == Tag::PLAYER)
        {
            continue;
        }

        Damageable* damageableScript = GameObjectAPI::findScript<Damageable>(obj);
        if (damageableScript)
        {
            damageables.push_back(damageableScript);
        }
	}

    Vector3 flatForward = forward;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    const float halfAngleRadians = DirectX::XMConvertToRadians(m_config->m_volleyConeAngleDegrees * 0.5f);
    const float minDot = std::cos(halfAngleRadians);

    for (Damageable* damageable : damageables)
    {
        if (damageable == nullptr)
        {
            continue;
        }

        const Transform* enemyTransform = GameObjectAPI::getTransform(damageable->getOwner());

        if (enemyTransform == nullptr)
        {
            continue;
        }

        const Vector3 enemyPosition = TransformAPI::getGlobalPosition(enemyTransform);
        Vector3 toEnemy = enemyPosition - origin;
        toEnemy.y = 0.0f;

        const float distanceSq = toEnemy.LengthSquared();
        if (distanceSq <= 0.0001f)
        {
            continue;
        }

        if (distanceSq > (m_config->m_volleyRange * m_config->m_volleyRange))
        {
            continue;
        }

        toEnemy.Normalize();

        const float dot = flatForward.Dot(toEnemy);
        if (dot >= minDot)
        {
            outTargets.push_back(damageable);
        }
    }
}

bool LyrielArrowVolley::applyVolleyDamage(const std::vector<Damageable*>& targets)
{
    bool anyMarkExploited = false;

    for (Damageable* target : targets)
    {
        if (target == nullptr)
        {
            continue;
        }

        ////habra q hacer refactor de damageable pq esto no es del todo eficiente
        //EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(target);

        //if (damageable != nullptr)
        //{
        //    damageable->takeDamageEnemy(m_volleyDamage, GameObjectAPI::getTransform(getOwner()));
        //}

        //else
        //{
        //    BreakableDamageable* breakableDamageable = GameObjectAPI::findScript<BreakableDamageable>(target);

        //    if (breakableDamageable != nullptr)
        //    {
        //        breakableDamageable->takeDamage(m_volleyDamage);
        //    }
        //}

        if(EnemyDamageable* enemyDamageable = dynamic_cast<EnemyDamageable*>(target))
        {
            EnemyHitContext ctx;
            ctx.damage = m_config->m_volleyDamage;
            ctx.attacker = GameObjectAPI::getTransform(getOwner());
            ctx.attackType = EnemyAttackType::LyrielVolley;
            enemyDamageable->takeDamage(ctx);
        }
        else if(BreakableDamageable* breakableDamageable = dynamic_cast<BreakableDamageable*>(target))
        {
            breakableDamageable->takeDamage(m_config->m_volleyDamage);
		}

        if (PersistingPowerupState::isUnlocked(PowerupId::LyrielPowerup1))
        {
            EnemyShadowMark* mark = GameObjectAPI::findScript<EnemyShadowMark>(target->getOwner());

            if (mark != nullptr && mark->isExploitable())
            {
                mark->exploit();
                anyMarkExploited = true;
                if (m_lyrielCharacter != nullptr)
                    m_lyrielCharacter->onMarkExploited();
            }
        }
    }

    return anyMarkExploited;
}

void LyrielArrowVolley::spawnVolleyArrows(const Vector3& origin, const Vector3& forward)
{
    if (m_lyrielCharacter == nullptr || m_config->m_volleyNumVisualArrows <= 0)
    {
        return;
    }

    ArrowPool* arrowPool = m_lyrielCharacter->getArrowPool();
    if (arrowPool == nullptr)
    {
        return;
    }

    Vector3 flatForward = forward;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    const float totalAngle = m_config->m_volleyConeAngleDegrees;

    float lifetime = 0.0f;
    if (m_config->m_volleyArrowSpeed > 0.0001f)
    {
        lifetime = m_config->m_volleyRange / m_config->m_volleyArrowSpeed;
    }

    for (int i = 0; i < m_config->m_volleyNumVisualArrows; ++i)
    {
        LyrielArrowProjectile* arrow = arrowPool->acquireArrow();
        if (arrow == nullptr)
        {
            Debug::log("[LyrielArrowVolley] No available arrow in pool for visual arrow %d.", i);
            return;
        }

        float t = 0.5f;
        if (m_config->m_volleyNumVisualArrows > 1)
        {
            t = static_cast<float>(i) / static_cast<float>(m_config->m_volleyNumVisualArrows - 1);
        }

        const float angleOffset = -totalAngle * 0.5f + totalAngle * t;
        const float angleRad = DirectX::XMConvertToRadians(angleOffset);

        const float cosA = std::cos(angleRad);
        const float sinA = std::sin(angleRad);

        Vector3 dir;
        dir.x = flatForward.x * cosA - flatForward.z * sinA;
        dir.y = 0.0f;
        dir.z = flatForward.x * sinA + flatForward.z * cosA;

        if (dir.LengthSquared() <= 0.0001f)
        {
            dir = flatForward;
        }
        else
        {
            dir.Normalize();
        }

        arrow->launch(origin, dir, m_config->m_volleyArrowSpeed, lifetime, nullptr, 0.0f);
    }
}

void LyrielArrowVolley::drawAimPreview(const Vector3& origin, const Vector3& forward) const
{
    Vector3 flatForward = forward;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    const float halfAngleRad = DirectX::XMConvertToRadians(m_config->m_volleyConeAngleDegrees * 0.5f);
    const int arcSteps = 16;

    const Vector3 previewColor(0.2f, 1.0f, 0.2f);

    auto rotateXZ = [](const Vector3& dir, float angleRad) -> Vector3
        {
            const float c = std::cos(angleRad);
            const float s = std::sin(angleRad);

            Vector3 result;
            result.x = dir.x * c - dir.z * s;
            result.y = 0.0f;
            result.z = dir.x * s + dir.z * c;
            return result;
        };

    Vector3 leftDir = rotateXZ(flatForward, -halfAngleRad);
    Vector3 rightDir = rotateXZ(flatForward, halfAngleRad);

    leftDir.Normalize();
    rightDir.Normalize();

    DebugDrawAPI::drawLine(origin, origin + leftDir * m_config->m_volleyRange, previewColor, 0, true);
    DebugDrawAPI::drawLine(origin, origin + rightDir * m_config->m_volleyRange, previewColor, 0, true);

    Vector3 previousPoint = origin + leftDir * m_config->m_volleyRange;

    for (int i = 1; i <= arcSteps; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(arcSteps);
        const float angle = -halfAngleRad + (2.0f * halfAngleRad * t);

        Vector3 arcDir = rotateXZ(flatForward, angle);
        arcDir.Normalize();

        Vector3 currentPoint = origin + arcDir * m_config->m_volleyRange;

        DebugDrawAPI::drawLine(previousPoint, currentPoint, previewColor, 0, true);
        previousPoint = currentPoint;
    }
}

IMPLEMENT_SCRIPT(LyrielArrowVolley)
