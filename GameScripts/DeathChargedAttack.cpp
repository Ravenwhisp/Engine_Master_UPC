#include "pch.h"
#include "DeathChargedAttack.h"

#include "DeathCharacter.h"
#include "DeathSound.h"
#include "PlayerAnimationController.h"
#include "CharacterAnimations.h"
#include "PlayerState.h"
#include "EnemyDamageable.h"
#include "BreakableDamageable.h"
#include "DeathUI.h"
#include "DeathConfig.h"
#include "DeathParticles.h"

#include <cmath>

// When fully charged, keep the charge held (aiming) for this long before auto-releasing.
static constexpr float k_maxChargeHoldGrace = 2.0f;

DeathChargedAttack::DeathChargedAttack(GameObject* owner)
    : ChargedAttackBase(owner)
{
}

void DeathChargedAttack::Start()
{
    ChargedAttackBase::Start();

    m_deathCharacter = dynamic_cast<DeathCharacter*>(m_character);
    m_config = m_deathCharacter->getConfig();
    m_deathUI = GameObjectAPI::findScript<DeathUI>(getOwner());
    m_particles = GameObjectAPI::findScript<DeathParticles>(getOwner());

    if (m_deathCharacter == nullptr)
        Debug::error("[DeathChargedAttack] Owner does not have a valid DeathCharacter.");
    if (m_config == nullptr)
        Debug::error("[DeathChargedAttack] DeathConfig not found.");
    if (m_deathUI == nullptr)
        Debug::warn("[DeathChargedAttack] DeathUI not found.");
    if (m_particles == nullptr)
        Debug::error("[DeathChargedAttack] DeathParticles not found.");
}

void DeathChargedAttack::Update()
{
    ChargedAttackBase::Update();

    if (m_isCharging)
    {
        m_chargeTime += Time::getDeltaTime();

        const bool isMaxCharge = m_chargeTime >= m_config->m_chargedMaxChargeTime;
        if (m_chargeTime > m_config->m_chargedMaxChargeTime)
            m_chargeTime = m_config->m_chargedMaxChargeTime;

        if (isMaxCharge)
            m_maxHoldTimer += Time::getDeltaTime();

        const float chargeRatio = m_config->m_chargedMaxChargeTime > 0.0f
            ? (m_chargeTime / m_config->m_chargedMaxChargeTime) : 1.0f;

        if (m_character != nullptr)
        {
            PlayerAnimationController* anim = m_character->getAnimationController();
            if (anim != nullptr)
                anim->setChargeProgress(chargeRatio);
        }

        if (m_deathUI)
        {
            Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
            if (ownerTransform)
            {
                const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);
                m_deathUI->showChargedAttackUI();
                m_deathUI->updateChargedAttackUI(origin, chargeRatio, m_config->m_chargedCircleRadius, isMaxCharge);
            }
        }

        if (Input::isRightTriggerReleased(getPlayerIndex()) || m_maxHoldTimer >= k_maxChargeHoldGrace)
        {
            fireAttack();
        }

        return;
    }

    if (m_deathUI)
        m_deathUI->hideChargedAttackUI();
}

void DeathChargedAttack::startAbility()
{
    startCharging();
}

bool DeathChargedAttack::canStartSpecificAbility() const
{
    return m_deathCharacter != nullptr;
}

void DeathChargedAttack::startCharging()
{
    m_chargeTime   = 0.0f;
    m_maxHoldTimer = 0.0f;
    m_isCharging   = true;

    setAbilityLocked(true);
    applyChargingMovementSlowdown(m_config->m_chargedMovementSlowdownPercentage);

    if (m_attackAnims != nullptr && m_character != nullptr)
    {
        PlayerAnimationController* anim = m_character->getAnimationController();
        if (anim != nullptr)
        {
            const AttackAnimInfo info = m_attackAnims->resolve(AttackAnimId::Charged, 0);
            if (!info.stateName.empty())
            {
                anim->beginChargeHold(info.stateName, info.blendIn, info.speed, info.holdPct);
            }
        }
    }
    
    if (m_particles != nullptr)
    {
        m_particles->SetChargeActive();
    }

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
    if (sound != nullptr)
        sound->startChargeLoop();
}

void DeathChargedAttack::fireAttack()
{
    const bool  isMaxCharge   = (m_chargeTime >= m_config->m_chargedMaxChargeTime);
    const bool  isChargedShot = (m_chargeTime >= m_config->m_chargedMinChargeTime);

    float damage;
    if (isChargedShot)
    {
        const float rawRatio    = m_chargeTime / m_config->m_chargedMaxChargeTime;
        const float chargeRatio = rawRatio > 1.0f ? 1.0f : rawRatio;
        damage = m_config->m_chargedAttackDamage + (m_config->m_chargedMaxChargeDamage - m_config->m_chargedAttackDamage) * chargeRatio;

        Debug::log("[ChargedAttack] Charged shot  dmg=%.1f  ratio=%.0f%%%s",
            damage, chargeRatio * 100.0f, isMaxCharge ? " (MAX)" : "");
    }
    else
    {
        damage = m_config->m_chargedAttackDamage;
        Debug::log("[ChargedAttack] Quick tap  dmg=%.1f", damage);
    }

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->stopChargeLoop();
        if (isChargedShot)
            sound->playChargeRelease();
        else
            sound->playHeavySwing();
    }

    m_pendingDamage = damage;
    m_pendingRadius = m_config->m_chargedCircleRadius;
    m_pendingChargedShot = isChargedShot;
    m_pendingMaxCharge = isMaxCharge;

    notifyAbilitySuccessfullyStarted();

    m_isCharging = false;

    if (m_particles != nullptr)
    {
        m_particles->SetChargeInactive();
    }

    resetChargingMovementSlowdown();
    m_chargeTime = 0.0f;

    if (m_character != nullptr)
    {
        PlayerAnimationController* anim = m_character->getAnimationController();
        if (anim != nullptr)
        {
            // Always play the full spin (circular damage); charge only adds follow-through.
            const float chargeRatio = m_config->m_chargedMaxChargeTime > 0.0f
                ? (m_chargeTime / m_config->m_chargedMaxChargeTime) : 1.0f;
            anim->endChargeHold(0.55f + 0.45f * chargeRatio);
        }

        PlayerState* playerState = m_character->getPlayerState();
        if (playerState != nullptr && !playerState->isDowned())
            playerState->setState(PlayerStateType::AttackRecovery);
    }

    resolveCurrentAttackAnim();
    beginAttackWindow(m_config->m_chargedAttackLockDuration);
    startCooldown();
}

void DeathChargedAttack::dealDamageInCircle(float damage, float radius, bool isChargedShot, bool isMaxCharge) const
{
    const Transform* myTransform = GameObjectAPI::getTransform(m_owner);
    if (myTransform == nullptr) return;

    const Vector3 myPos = TransformAPI::getGlobalPosition(myTransform);
    const float radiusSq = radius * radius;

    const auto enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY);
    const auto breakables = SceneAPI::findAllGameObjectsByTag(Tag::BREAKABLE);
    auto targets = enemies;
    targets.insert(targets.end(), breakables.begin(), breakables.end());
    int scanned = 0;
    int hit = 0;

    for (GameObject* target : targets)
    {
        if (target == nullptr) continue;

        const Transform* enemyTr = GameObjectAPI::getTransform(target);
        if (enemyTr == nullptr) continue;

        scanned++;

        Vector3 toEnemy = TransformAPI::getGlobalPosition(enemyTr) - myPos;
        toEnemy.y = 0.0f;

        if (toEnemy.LengthSquared() > radiusSq) continue;

        EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(target);
        if (damageable == nullptr)
        {
            BreakableDamageable* breakableDamageable = GameObjectAPI::findScript<BreakableDamageable>(target);
            if (breakableDamageable == nullptr)
            {
                Debug::log("[CIRCLE] '%s' has no Damageable.", GameObjectAPI::getName(target));
                continue;
            }
            breakableDamageable->takeDamage(damage);
        }
        else
        {
            EnemyHitContext ctx;
            ctx.damage = damage;
            ctx.attacker = GameObjectAPI::getTransform(getOwner());
            ctx.attackType = PlayerAttackType::DeathCharged;
            damageable->takeDamage(ctx);

            tryStunTarget(target, isMaxCharge, m_config->m_chargedStunOnMaxCharge, m_config->m_chargedStunDuration);
        }

        hit++;
    }

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
    if (sound != nullptr && hit > 0 && !isChargedShot)
        sound->playHeavyImpact();

    if (scanned == 0)
        Debug::log("[CIRCLE] no ENEMY tagged objects in scene.");
    else if (hit == 0)
        Debug::log("[CIRCLE] 0 hits — %d enemies scanned, none in range.", scanned);
}

void DeathChargedAttack::onAttackWindowUpdate()
{
    if (m_particles != nullptr)
        m_particles->SetScytheActive();
}

void DeathChargedAttack::onAttackWindowFinished()
{
    if (m_particles != nullptr)
        m_particles->SetScytheInactive();
}

void DeathChargedAttack::onHitFrame()
{
    dealDamageInCircle(m_pendingDamage, m_pendingRadius, m_pendingChargedShot, m_pendingMaxCharge);
}

float DeathChargedAttack::getCooldown() const
{
    return m_deathCharacter->getConfig()->m_chargedCooldown;
}

void DeathChargedAttack::drawGizmo()
{
    if (m_deathCharacter == nullptr) return;

    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr) return;

    const Vector3 pos     = TransformAPI::getGlobalPosition(t);
    const float   radius  = m_deathCharacter->getConfig()->m_chargedCircleRadius;
    const Vector3 posFlat = { pos.x, pos.y, pos.z };

    const Vector3 colGrey   = { 0.35f, 0.35f, 0.35f };
    const Vector3 colOrange = { 1.0f,  0.55f, 0.0f  };
    const Vector3 colYellow = { 1.0f,  0.9f,  0.0f  };
    const Vector3 colBase   = m_isCharging ? colOrange : colGrey;

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    constexpr int   circleSegs = 24;
    const float     segAngle   = 360.0f / static_cast<float>(circleSegs) * k_degToRad;

    auto circlePoint = [&](int i) -> Vector3
    {
        const float a = segAngle * static_cast<float>(i);
        return Vector3(cosf(a) * radius, 0.0f, sinf(a) * radius);
    };

    // Circle outline
    for (int i = 0; i < circleSegs; ++i)
    {
        const int next = (i + 1) % circleSegs;
        DebugDrawAPI::drawLine(posFlat + circlePoint(i), posFlat + circlePoint(next), colBase);
    }

    // Charge fill: yellow arcs that grow with charge ratio
    if (m_isCharging && m_config->m_chargedMaxChargeTime > 0.0f)
    {
        const float ratio   = m_chargeTime / m_config->m_chargedMaxChargeTime;
        const float clamped = ratio > 1.0f ? 1.0f : ratio;
        const int   fillEnd = static_cast<int>(clamped * static_cast<float>(circleSegs));

        for (int i = 0; i < fillEnd; ++i)
        {
            const int next = (i + 1) % circleSegs;
            DebugDrawAPI::drawLine(posFlat + circlePoint(i), posFlat + circlePoint(next), colYellow);
        }
    }
}

void DeathChargedAttack::updateUI()
{
    AbilityBase::updateUI();

    if (m_deathUI)
        m_deathUI->updateChargedSlashUI(m_attackStateTimer, m_deathCharacter->getConfig()->m_chargedAttackLockDuration);
}

IMPLEMENT_SCRIPT(DeathChargedAttack)
