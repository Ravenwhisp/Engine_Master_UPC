#include "pch.h"
#include "LyrielArrowVolley.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "CharacterBase.h"
#include "ProjectilePool.h"
#include "LyrielArrowProjectile.h"
#include "EnemyDamageable.h"
#include "PlayerState.h"
#include "PersistingPowerupState.h"
#include "EnemyShadowMark.h"
#include "BreakableDamageable.h"
#include "LyrielUI.h"
#include "LyrielConfig.h"

IMPLEMENT_SCRIPT_FIELDS(LyrielArrowVolley,
    SERIALIZED_ASSET_REF(m_config, "Lyriel Config", AssetType::DATA_CONTAINER)
)

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
    const LyrielConfig* cfg = m_config.get();
    return cfg ? cfg->m_volleyCooldown : 0.0f;
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
    notifyAbilitySuccessfullyStarted();

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

    const LyrielConfig* cfgLock = m_config.get();
    if (cfgLock)
    {
        beginAttackWindow(cfgLock->m_volleyAttackLockDuration);
    }
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

    const LyrielConfig* cfg = m_config.get();
    if (!cfg) return;

    const std::vector<GameObject*> objectsInCircularRange = SceneAPI::getObjectsInCircularArea(Vector2(origin.x, origin.z), cfg->m_volleyRange);

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

    const float halfAngleRadians = DirectX::XMConvertToRadians(cfg->m_volleyConeAngleDegrees * 0.5f);
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

        if (distanceSq > (cfg->m_volleyRange * cfg->m_volleyRange))
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

    const LyrielConfig* cfg = m_config.get();
    if (!cfg) return false;

    for (Damageable* target : targets)
    {
        if (target == nullptr)
        {
            continue;
        }

        if(EnemyDamageable* enemyDamageable = dynamic_cast<EnemyDamageable*>(target))
        {
            EnemyHitContext ctx;
            ctx.damage = cfg->m_volleyDamage;
            ctx.attacker = GameObjectAPI::getTransform(getOwner());

            if (PersistingPowerupState::isUnlocked(PowerupId::LyrielPowerup1))
            {
                EnemyShadowMark* mark = GameObjectAPI::findScript<EnemyShadowMark>(target->getOwner());

                if (mark != nullptr && mark->isExploitable())
                {
                    mark->exploit();
					ctx.attackType = EnemyAttackType::ShadowMarkExploit;
                    anyMarkExploited = true;
                    if (m_lyrielCharacter != nullptr)
                        m_lyrielCharacter->onMarkExploited();
                }
                else
                {
                    ctx.attackType = EnemyAttackType::LyrielVolley;
                }
            }

            enemyDamageable->takeDamage(ctx);
            continue;
        }
        else if(BreakableDamageable* breakableDamageable = dynamic_cast<BreakableDamageable*>(target))
        {
            breakableDamageable->takeDamage(cfg->m_volleyDamage);
		} 
    }

    return anyMarkExploited;
}

void LyrielArrowVolley::spawnVolleyArrows(const Vector3& origin, const Vector3& forward)
{
    const LyrielConfig* cfg = m_config.get();
    if (m_lyrielCharacter == nullptr || !cfg || cfg->m_volleyNumVisualArrows <= 0)
    {
        return;
    }

    ProjectilePool* projectilePool = m_lyrielCharacter->getArrowPool();
    if (projectilePool == nullptr)
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

    const float totalAngle = cfg->m_volleyConeAngleDegrees;

    float lifetime = 0.0f;
    if (cfg->m_volleyArrowSpeed > 0.0001f)
    {
        lifetime = cfg->m_volleyRange / cfg->m_volleyArrowSpeed;
    }

    for (int i = 0; i < cfg->m_volleyNumVisualArrows; ++i)
    {
        ProjectileBase* projectile = projectilePool->acquireProjectile();
        if (!projectile)
        {
            Debug::log("[LyrielArrowVolley] No available projectile in pool for visual arrow %d.", i);
            return;
        }

        LyrielArrowProjectile* arrow = static_cast<LyrielArrowProjectile*>(projectile);

        float t = 0.5f;
        if (cfg->m_volleyNumVisualArrows > 1)
        {
            t = static_cast<float>(i) / static_cast<float>(cfg->m_volleyNumVisualArrows - 1);
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

        arrow->launch(origin, dir, cfg->m_volleyArrowSpeed, lifetime, nullptr, 0.0f);
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

    const LyrielConfig* cfg = m_config.get();
    if (!cfg) return;

    const float halfAngleRad = DirectX::XMConvertToRadians(cfg->m_volleyConeAngleDegrees * 0.5f);
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

    DebugDrawAPI::drawLine(origin, origin + leftDir * cfg->m_volleyRange, previewColor, 0, true);
    DebugDrawAPI::drawLine(origin, origin + rightDir * cfg->m_volleyRange, previewColor, 0, true);

    Vector3 previousPoint = origin + leftDir * cfg->m_volleyRange;

    for (int i = 1; i <= arcSteps; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(arcSteps);
        const float angle = -halfAngleRad + (2.0f * halfAngleRad * t);

        Vector3 arcDir = rotateXZ(flatForward, angle);
        arcDir.Normalize();

        Vector3 currentPoint = origin + arcDir * cfg->m_volleyRange;

        DebugDrawAPI::drawLine(previousPoint, currentPoint, previewColor, 0, true);
        previousPoint = currentPoint;
    }
}

IMPLEMENT_SCRIPT(LyrielArrowVolley)
