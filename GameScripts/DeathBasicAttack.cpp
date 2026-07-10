#include "pch.h"
#include "DeathBasicAttack.h"

#include "DeathCharacter.h"
#include "DeathSound.h"
#include "PlayerTargetController.h"
#include "PlayerAnimationController.h"
#include "PlayerRotation.h"
#include "PlayerState.h"
#include "EnemyDamageable.h"
#include "EnemyShadowMark.h"
#include "BreakableDamageable.h"
#include "DeathUI.h"
#include "DeathConfig.h"
#include "DeathParticles.h"

#include <cmath>

DeathBasicAttack::DeathBasicAttack(GameObject* owner)
    : DeathAbilityBase(owner)
{
}

void DeathBasicAttack::Start()
{
    DeathAbilityBase::Start();

    m_deathUI = GameObjectAPI::findScript<DeathUI>(getOwner());

    if (!m_deathUI)
    {
        Debug::warn("[DeathBasicAttack] DeathUI not found.");
    }

    m_particles = GameObjectAPI::findScript<DeathParticles>(getOwner());

    if (!m_particles)
    {
        Debug::error("[DeathBasicAttack] DeathParticles not found.");
        return;
    }

}

void DeathBasicAttack::Update()
{
    DeathAbilityBase::Update();

    // Block new input while the attack window is still running
    if (m_attackStateTimer > 0.0f)
    {
        return;
    }
}

bool DeathBasicAttack::canStartSpecificAbility() const
{
    return m_deathCharacter != nullptr && !m_deathCharacter->isInComboCooldown();
}

void DeathBasicAttack::startAbility()
{
    GameObject* target = m_character->getTargetController()
        ? m_character->getTargetController()->getCurrentTarget()
        : nullptr;

    setAbilityLocked(true);
    m_movementLockedForCombo = true;

    snapFaceTarget(target);
    m_attackFacingTarget = target;

    const int comboStep = m_deathCharacter->getComboStep();

    DeathSound* sound = m_deathCharacter->getSound();
    if (sound != nullptr)
    {
        sound->playLightSwing();
    }

    dealDamageToTarget(target);
    notifyAbilitySuccessfullyStarted();

    m_deathCharacter->advanceCombo(false);

    const DeathConfig* cfg = m_config.get();

    const bool  isFinalHit  = (comboStep >= 2);
    const float lockDuration = isFinalHit ? cfg->m_basicFinalHitLockDuration : cfg->m_basicAttackLockDuration;

    beginAttackPresentation();
    beginAttackWindow(lockDuration);
    startCooldown();

    if (!isFinalHit)
    {
        Debug::log("[R1] step %d/3", comboStep + 1);
    }
    else
    {
        Debug::log("[R1] step 3/3 - combo complete");
    }
}

void DeathBasicAttack::onAttackWindowUpdate()
{
    faceTarget(m_attackFacingTarget);

    PlayerAnimationController* anim = m_character ? m_character->getAnimationController() : nullptr;
    if (anim != nullptr)
    {
        anim->requestAttack();
    }

    if (m_particles != nullptr)
    {
        m_particles->SetScytheActive();
    }
}

void DeathBasicAttack::onAttackWindowFinished()
{
    m_attackFacingTarget = nullptr;

    if (m_movementLockedForCombo)
    {
        releaseComboMoveLock();
    }

    if (m_particles != nullptr)
    {
        m_particles->SetScytheInactive();
    }
}

float DeathBasicAttack::getCooldown() const
{
    const DeathConfig* cfg = m_config.get();
    if (!cfg) return 0.0f;
    return cfg->m_basicCooldown;
}

void DeathBasicAttack::dealDamageToTarget(GameObject* target) const
{
    const Transform* myTransform = GameObjectAPI::getTransform(m_owner);
    if (myTransform == nullptr)
    {
        return;
    }

    const Vector3 myPos = TransformAPI::getGlobalPosition(myTransform);
    Vector3 myForward = TransformAPI::getForward(myTransform);
    myForward.y = 0.0f;
    const float fwdLen = myForward.Length();
    if (fwdLen > 0.0001f)
    {
        myForward /= fwdLen;
    }

    constexpr float k_degToRad = 3.14159265f / 180.0f;

    const DeathConfig* cfg = m_config.get();
    if (!cfg) return;

    const float halfHitCos = cosf(cfg->m_basicAttackHitAngle * 0.5f * k_degToRad);
    const float rangeSq = cfg->m_basicAttackRange * cfg->m_basicAttackRange;

    auto isInHitZone = [&](GameObject* enemy) -> bool
        {
            if (enemy == nullptr)
            {
                return false;
            }
            const Transform* eTr = GameObjectAPI::getTransform(enemy);
            if (eTr == nullptr)
            {
                return false;
            }
            Vector3 toE = TransformAPI::getGlobalPosition(eTr) - myPos;
            toE.y = 0.0f;
            if (toE.LengthSquared() > rangeSq)
            {
                return false;
            }
            if (toE.LengthSquared() > 0.0001f)
            {
                Vector3 toENorm = toE;
                toENorm.Normalize();
                if (myForward.Dot(toENorm) < halfHitCos)
                {
                    return false;
                }
            }
            return true;
        };

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;

    auto applyDamage = [&](GameObject* enemy)
        {
            EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(enemy);
            if (damageable == nullptr)
            {
                BreakableDamageable* breakableDamageable = GameObjectAPI::findScript<BreakableDamageable>(enemy);
                if(breakableDamageable == nullptr)
                {
                    return;
                }
                breakableDamageable->takeDamage(cfg->m_basicAttackDamage);
                if (sound != nullptr)
                {
                    sound->playLightImpact();
                }
                return;
            }

            {
                EnemyHitContext ctx;
                ctx.damage = cfg->m_basicAttackDamage;
                ctx.attacker = GameObjectAPI::getTransform(getOwner());
                ctx.attackType = EnemyAttackType::DeathBasic;
                damageable->takeDamage(ctx);
            }

            Debug::log("[BASIC] hit '%s'  dmg=%.1f  hp=%.1f/%.1f",
                GameObjectAPI::getName(enemy),
                cfg->m_basicAttackDamage,
                damageable->getCurrentHp(),
                damageable->getMaxHp());

            if (sound != nullptr)
            {
                sound->playLightImpact();
            }

            EnemyShadowMark* shadowMark = GameObjectAPI::findScript<EnemyShadowMark>(enemy);
            if (shadowMark != nullptr)
            {
                shadowMark->notifyDeathHit();
                if (sound != nullptr)
                {
                    sound->playMarkApply();
                }
            }
        };

    // Priority 1: targeted enemy in hit zone
    if (target != nullptr && isInHitZone(target))
    {
        applyDamage(target);
        return;
    }

    // Priority 2: most-centered enemy in hit zone (no target or target out of zone)
    float       bestDot = -2.0f;
    GameObject* best = nullptr;
    for (GameObject* enemy : SceneAPI::findAllGameObjectsByTag(Tag::ENEMY))
    {
        if (!isInHitZone(enemy))
        {
            continue;
        }
        const Transform* eTr = GameObjectAPI::getTransform(enemy);
        Vector3 toE = TransformAPI::getGlobalPosition(eTr) - myPos;
        toE.y = 0.0f;
        toE.Normalize();
        const float dot = myForward.Dot(toE);
        if (dot > bestDot)
        {
            bestDot = dot;
            best = enemy;
        }
    }

    if (best != nullptr)
    {
        applyDamage(best);
    }
    else
    {
        Debug::log("[BASIC] 0 hits — no enemy in hit zone.");
    }
}

void DeathBasicAttack::drawGizmo()
{
    if (m_deathCharacter == nullptr)
    {
        return;
    }

    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr)
    {
        return;
    }

    const Vector3 pos      = TransformAPI::getGlobalPosition(t);
    const Vector3 fwd      = TransformAPI::getForward(t);
    const float   fill     = m_deathCharacter->getComboFillRatio();

    const DeathConfig* cfg = m_config.get();
    if (!cfg) return;

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    const float hitHalfRad     = cfg->m_basicAttackHitAngle * 0.5f * k_degToRad;

    const Vector3 posFlat = { pos.x, pos.y, pos.z };

    const Vector3 colGrey   = { 0.35f, 0.35f, 0.35f };
    const Vector3 colPurple = { 0.9f,  0.0f,  0.9f  };
    const Vector3 colOrange = { 1.0f,  0.55f, 0.0f  };
    const Vector3 colFill   = m_deathCharacter->wasLastHitR2() ? colOrange : colPurple;

    auto radialDir = [&](float a) -> Vector3
    {
        return Vector3(
            fwd.x * cosf(a) + fwd.z * sinf(a),
            0.0f,
            -fwd.x * sinf(a) + fwd.z * cosf(a));
    };

    // Detection range: full circle
    const int   circleSegs = 32;
    const float circleStep = (2.0f * 3.14159265f) / static_cast<float>(circleSegs);
    for (int i = 0; i < circleSegs; ++i)
    {
        const float a0 = circleStep * static_cast<float>(i);
        const float a1 = a0 + circleStep;
        DebugDrawAPI::drawLine(posFlat + radialDir(a0) * cfg->m_basicAttackRange,
                               posFlat + radialDir(a1) * cfg->m_basicAttackRange, colGrey);
    }

    // Hit zone: narrow arc with edge lines
    const Vector3 hitLeft  = radialDir(-hitHalfRad);
    const Vector3 hitRight = radialDir( hitHalfRad);
    DebugDrawAPI::drawLine(posFlat, posFlat + hitLeft  * cfg->m_basicAttackRange, colGrey);
    DebugDrawAPI::drawLine(posFlat, posFlat + hitRight * cfg->m_basicAttackRange, colGrey);

    const int   arcSegs = 12;
    const float arcStep = (cfg->m_basicAttackHitAngle * k_degToRad) / static_cast<float>(arcSegs);
    for (int i = 0; i < arcSegs; ++i)
    {
        const float a0 = -hitHalfRad + arcStep * static_cast<float>(i);
        const float a1 = a0 + arcStep;
        DebugDrawAPI::drawLine(posFlat + radialDir(a0) * cfg->m_basicAttackRange,
                               posFlat + radialDir(a1) * cfg->m_basicAttackRange, colGrey);
    }

    // Combo fill: purple (R1 last) or orange (R2 last) on hit zone arc
    if (fill > 0.0f)
    {
        const int   fillLines   = 16;
        const float filledAngle = fill * 2.0f * hitHalfRad;

        for (int i = 0; i <= fillLines; ++i)
        {
            const float t2 = static_cast<float>(i) / static_cast<float>(fillLines);
            const float a  = -hitHalfRad + t2 * filledAngle;
            DebugDrawAPI::drawLine(posFlat, posFlat + radialDir(a) * cfg->m_basicAttackRange, colFill);
        }

        DebugDrawAPI::drawLine(posFlat, posFlat + hitLeft * cfg->m_basicAttackRange, colFill);
    }
}

void DeathBasicAttack::snapFaceTarget(GameObject* target)
{
    if (target == nullptr)
    {
        return;
    }

    const DeathConfig* cfg = m_config.get();
    if (!cfg) return;

    Transform* myTransform     = GameObjectAPI::getTransform(getOwner());
    Transform* targetTransform = GameObjectAPI::getTransform(target);
    if (myTransform == nullptr || targetTransform == nullptr)
    {
        return;
    }

    const float rangeSq = cfg->m_basicAttackRange * cfg->m_basicAttackRange;

    Vector3 myPos     = TransformAPI::getGlobalPosition(myTransform);
    Vector3 targetPos = TransformAPI::getGlobalPosition(targetTransform);
    Vector3 dir       = targetPos - myPos;
    dir.y = 0.0f;

    if (dir.LengthSquared() > rangeSq || dir.LengthSquared() <= 0.0001f)
    {
        return;
    }

    dir.Normalize();

    constexpr float k_radToDeg = 180.0f / 3.14159265f;
    const float     yaw        = atan2f(dir.x, dir.z) * k_radToDeg;
    const Vector3   euler      = TransformAPI::getGlobalEulerDegrees(myTransform);
    TransformAPI::setGlobalRotationEuler(myTransform, Vector3(euler.x, yaw, euler.z));
}

void DeathBasicAttack::faceTarget(GameObject* target)
{
    PlayerRotation* playerRotation = m_character ? m_character->getPlayerRotation() : nullptr;
    if (playerRotation == nullptr || target == nullptr)
    {
        return;
    }

    const DeathConfig* cfg = m_config.get();
    if (!cfg) return;

    Transform* myTransform     = GameObjectAPI::getTransform(getOwner());
    Transform* targetTransform = GameObjectAPI::getTransform(target);

    Vector3 myPos     = TransformAPI::getGlobalPosition(myTransform);
    Vector3 targetPos = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 dir = targetPos - myPos;
    dir.y = 0.0f;

    const float rangeSq = cfg->m_basicAttackRange * cfg->m_basicAttackRange;
    if (dir.LengthSquared() > rangeSq)
    {
        return;
    }

    if (dir.LengthSquared() <= 0.0001f)
    {
        return;
    }
    dir.Normalize();

    playerRotation->applyFacingFromDirection(getOwner(), dir, Time::getDeltaTime());
}

void DeathBasicAttack::updateUI()
{
    DeathAbilityBase::updateUI();

    if (m_deathUI)
    {
        const DeathConfig* cfg = m_config.get();
        if (!cfg) return;
        m_deathUI->updateBasicSlashUI(m_attackStateTimer, cfg->m_basicAttackLockDuration);
    }
}

IMPLEMENT_SCRIPT_FIELDS_INHERITED(DeathBasicAttack, DeathAbilityBase,
    SERIALIZED_ASSET_REF(m_config, "Death Config", AssetType::DATA_CONTAINER)
)

IMPLEMENT_SCRIPT(DeathBasicAttack)
