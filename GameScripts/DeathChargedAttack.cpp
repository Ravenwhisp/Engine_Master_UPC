#include "pch.h"
#include "DeathChargedAttack.h"

#include "DeathCharacter.h"
#include "DeathSound.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"
#include "EnemyDamageable.h"
#include "EnemyShadowMark.h"
#include "BreakableDamageable.h"
#include "DeathUI.h"
#include "EnemyBaseController.h"
#include "DeathConfig.h"
#include "DeathParticles.h"

#include <cmath>

DeathChargedAttack::DeathChargedAttack(GameObject* owner)
    : DeathAbilityBase(owner)
{
}

void DeathChargedAttack::Start()
{
    DeathAbilityBase::Start();

    m_deathUI = GameObjectAPI::findScript<DeathUI>(getOwner());

    if (!m_deathUI)
    {
        Debug::warn("[DeathChargedAttack] DeathUI not found.");
    }

    m_particles = GameObjectAPI::findScript<DeathParticles>(getOwner());

    if (!m_particles)
    {
        Debug::error("[DeathChargedAttack] DeathParticles not found.");
        return;
    }

}

void DeathChargedAttack::Update()
{
    DeathAbilityBase::Update();

    if (m_isCharging)
    {
        m_chargeTime += Time::getDeltaTime();
        updateAimDirection();

        if (m_deathUI)
        {
            Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

            if (ownerTransform)
            {
                const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);
                m_deathUI->showChargedAttackUI();
                m_deathUI->updateChargedAttackUI(origin, m_aimDirection);
            }
        }

        const bool maxReached = m_chargeTime >= m_config->m_chargedMaxChargeTime;
        const bool released = Input::isRightTriggerReleased(getPlayerIndex());

        if (maxReached || released)
        {
            fireAttack();
        }

        return;
    }

    if (m_deathUI)
    {
        m_deathUI->hideChargedAttackUI();
    }
}

void DeathChargedAttack::startAbility()
{
    startCharging();
}

bool DeathChargedAttack::canStartSpecificAbility() const
{
    return m_deathCharacter != nullptr && !m_deathCharacter->isInComboCooldown() && m_deathCharacter->canUseR2InCombo();
}

void DeathChargedAttack::startCharging()
{
    m_chargeTime             = 0.0f;
    m_isCharging             = true;
    m_aimDirection           = { 0.0f, 0.0f, 0.0f };
    m_movementLockedForCombo = true;

    setAbilityLocked(true);

    // Lock movement immediately — same Attacking state used by the basic attack combo lock
    PlayerState* ps = m_character->getPlayerState();
    if (ps != nullptr)
        ps->setState(PlayerStateType::AttackRecovery);

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->startChargeLoop();
    }

    Debug::log("[COMBO] R2 cargando  step=%d/3", m_deathCharacter->getComboStep() + 1);
}

void DeathChargedAttack::fireAttack()
{
    // Snap to the aim direction sampled during the hold, then deal damage in that direction
    snapFaceAimDirection();

    const int   comboStep   = m_deathCharacter->getComboStep();
    const bool  isMaxCharge = (m_chargeTime >= m_config->m_chargedMaxChargeTime);

    // Charged-mode shot: only valid as combo starter (step 0), needs min charge time
    const bool isChargedShot = (m_chargeTime >= m_config->m_chargedMinChargeTime) && (comboStep == 0);

    float damage;
    if (isChargedShot)
    {
        const float rawRatio    = m_chargeTime / m_config->m_chargedMaxChargeTime;
        const float chargeRatio = rawRatio > 1.0f ? 1.0f : rawRatio;
        damage = m_config->m_chargedAttackDamage * (1.0f + chargeRatio);

        if (isMaxCharge)
            Debug::log("[COMBO] R2 CARGA MAXIMA  step %d/3  dmg=%.1f", comboStep + 1, damage);
        else
            Debug::log("[COMBO] R2 CARGADO  step %d/3  ratio=%.0f%%  dmg=%.1f",
                comboStep + 1, (m_chargeTime / m_config->m_chargedMaxChargeTime) * 100.0f, damage);
    }
    else
    {
        damage = m_config->m_chargedAttackDamage;
        Debug::log("[COMBO] R2  step %d/3  dmg=%.1f", comboStep + 1, damage);
    }

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->stopChargeLoop();
        if (isChargedShot)
        {
            // Charged: the release IS the attack sound. No swing, no impact.
            sound->playChargeRelease();
        }
        else
        {
            // Tap R2 or mid-combo: regular swing; impact will play if it lands.
            sound->playHeavySwing();
        }
    }

    const float range = isChargedShot ? m_config->m_chargedShotArcRange : m_config->m_chargedArcRange;
    const float angle = isChargedShot ? m_config->m_chargedShotArcAngle : m_config->m_chargedArcAngle;

    dealDamageInArc(damage, range, angle, isChargedShot, isMaxCharge);
    notifyAbilitySuccessfullyStarted();

    // Max charge (auto-fired at full charge, always step 0) gets longer combo window
    const float window = (isChargedShot && isMaxCharge)
        ? m_deathCharacter->getComboWindowMaxCharge()
        : m_deathCharacter->getComboWindowR2();
    m_deathCharacter->advanceCombo(true, window);

    const bool isLast = (comboStep >= 2);
    if (isLast)
        Debug::log("[COMBO] R2  step 3/3  COMPLETO — reset");

    m_chargeTime = 0.0f;
    m_isCharging = false;

    const float lockDuration = (comboStep >= 2) ? m_config->m_chargedFinalHitLockDuration : m_config->m_chargedAttackLockDuration;

    // Trigger attack animation and start the post-fire movement lock window
    beginAttackPresentation();
    beginAttackWindow(lockDuration);
    startCooldown();
}

void DeathChargedAttack::dealDamageInArc(float damage, float range, float angle, bool isChargedShot, bool isMaxCharge) const
{
    const Transform* myTransform = GameObjectAPI::getTransform(m_owner);
    if (myTransform == nullptr)
    {
        return;
    }

    Vector3 myPos = TransformAPI::getGlobalPosition(myTransform);
    Vector3 myForward = TransformAPI::getForward(myTransform);

    myForward.y = 0.0f;
    const float fwdLen = myForward.Length();
    if (fwdLen > 0.0001f)
    {
        myForward /= fwdLen;
    }

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    const float halfAngleCos = cosf(angle * 0.5f * k_degToRad);
    const float arcRangeSq = range * range;

    const auto enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY);
	const auto breakables = SceneAPI::findAllGameObjectsByTag(Tag::BREAKABLE);
	auto targets = enemies;
	targets.insert(targets.end(), breakables.begin(), breakables.end());
    int scanned = 0;
    int hit = 0;
    bool anyMark = false;

    for (GameObject* target : targets)
    {
        if (target == nullptr)
        {
            continue;
        }

        const Transform* enemyTr = GameObjectAPI::getTransform(target);
        if (enemyTr == nullptr)
        {
            continue;
        }

        scanned++;

        Vector3 toEnemy = TransformAPI::getGlobalPosition(enemyTr) - myPos;
        toEnemy.y = 0.0f;

        const float distSq = toEnemy.LengthSquared();
        if (distSq > arcRangeSq)
        {
            continue;
        }

        if (angle < 360.0f && distSq > 0.0001f)
        {
            Vector3 toEnemyNorm = toEnemy;
            toEnemyNorm.Normalize();

            if (myForward.Dot(toEnemyNorm) < halfAngleCos)
            {
                continue;
            }
        }

        EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(target);
        if (damageable == nullptr)
        {
            BreakableDamageable* breakableDamageable = GameObjectAPI::findScript<BreakableDamageable>(target);
            if (breakableDamageable == nullptr)
            {
                Debug::log("[ARC] '%s' has no Damageable.", GameObjectAPI::getName(target));
                continue;
            }
            breakableDamageable->takeDamage(damage);    
        }
        else 
        {
            EnemyHitContext ctx;
            ctx.damage = damage;
            ctx.attacker = GameObjectAPI::getTransform(getOwner());
            ctx.attackType = EnemyAttackType::DeathCharged;
            damageable->takeDamage(ctx);

        }
        hit++;

        EnemyBaseController* enemyController = GameObjectAPI::findScript<EnemyBaseController>(target);

        if (enemyController != nullptr && isMaxCharge)
        {
            enemyController->useStun();
        }

        EnemyShadowMark* shadowMark = GameObjectAPI::findScript<EnemyShadowMark>(target);
        if (shadowMark != nullptr)
        {
            shadowMark->notifyDeathHit();
            anyMark = true;
        }
    }

    DeathSound* sound = m_deathCharacter != nullptr ? m_deathCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        // Charged shot uses charge_release as its impact sound (posted in fireAttack);
        // only non-charged shots get heavy_impact here.
        if (hit > 0 && !isChargedShot)
        {
            sound->playHeavyImpact();
        }
        if (anyMark)
        {
            sound->playMarkApply();
        }
    }

    if (scanned == 0)
    {
        Debug::log("[ARC] no ENEMY tagged objects in scene.");
    }
    else if (hit == 0)
    {
        Debug::log("[ARC] 0 hits — %d enemies scanned, none in range/angle.", scanned);
    }
}

void DeathChargedAttack::updateAimDirection()
{
    Vector3 aimDirection = computeCameraRelativeAimDirection(0.09f);

    if (aimDirection.LengthSquared() > 0.0001f)
    {
        m_aimDirection = aimDirection;
    }
    else
    {
        m_aimDirection = getFallbackFacingDirection();
    }
}

void DeathChargedAttack::snapFaceAimDirection()
{
    // If no stick input was given during the charge, keep current facing
    if (m_aimDirection.LengthSquared() <= 0.0001f)
        return;

    Transform* myTransform = GameObjectAPI::getTransform(getOwner());
    if (myTransform == nullptr)
        return;

    Vector3 dir = m_aimDirection;
    dir.Normalize();

    constexpr float k_radToDeg = 180.0f / 3.14159265f;
    const float     yaw        = atan2f(dir.x, dir.z) * k_radToDeg;
    const Vector3   euler      = TransformAPI::getGlobalEulerDegrees(myTransform);
    TransformAPI::setGlobalRotationEuler(myTransform, Vector3(euler.x, yaw, euler.z));
}

void DeathChargedAttack::onAttackWindowUpdate()
{
    PlayerAnimationController* anim = m_character ? m_character->getAnimationController() : nullptr;
    if (anim != nullptr)
        anim->requestAttack();

    if (m_particles != nullptr)
    {
        m_particles->SetScytheActive();
    }
}

void DeathChargedAttack::onAttackWindowFinished()
{
    if (m_movementLockedForCombo)
    {
        releaseComboMoveLock();
    }

    if (m_particles != nullptr)
    {
        m_particles->SetScytheInactive();
    }
}

float DeathChargedAttack::getCooldown() const
{
    return m_config->m_chargedCooldown;
}

void DeathChargedAttack::drawGizmo()
{
    if (m_deathCharacter == nullptr)
        return;

    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr)
        return;

    const Vector3 pos   = TransformAPI::getGlobalPosition(t);
    const float   range = m_config->m_chargedArcRange;

    // While charging with stick input, show arc in aim direction; otherwise use character forward
    Vector3 fwd;
    if (m_isCharging && m_aimDirection.LengthSquared() > 0.0001f)
    {
        fwd = m_aimDirection;
        fwd.Normalize();
    }
    else
    {
        fwd = TransformAPI::getForward(t);
    }
    const float   angle   = m_config->m_chargedArcAngle;
    const Vector3 posFlat = { pos.x, pos.y, pos.z };

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    const float halfRad        = angle * 0.5f * k_degToRad;

    const Vector3 colGrey   = { 0.35f, 0.35f, 0.35f };
    const Vector3 colOrange = { 1.0f,  0.55f, 0.0f  };
    const Vector3 colYellow = { 1.0f,  0.9f,  0.0f  };
    const Vector3 colBase   = m_isCharging ? colOrange : colGrey;

    auto radialDir = [&](float a) -> Vector3
    {
        return Vector3(
            fwd.x * cosf(a) + fwd.z * sinf(a),
            0.0f,
            -fwd.x * sinf(a) + fwd.z * cosf(a));
    };

    // Arc outline
    DebugDrawAPI::drawLine(posFlat, posFlat + radialDir(-halfRad) * range, colBase);
    DebugDrawAPI::drawLine(posFlat, posFlat + radialDir( halfRad) * range, colBase);

    const int   arcSegs = 12;
    const float arcStep = (angle * k_degToRad) / static_cast<float>(arcSegs);
    for (int i = 0; i < arcSegs; ++i)
    {
        const float a0 = -halfRad + arcStep * static_cast<float>(i);
        const float a1 = a0 + arcStep;
        DebugDrawAPI::drawLine(posFlat + radialDir(a0) * range,
                               posFlat + radialDir(a1) * range, colBase);
    }

    // Charge fill: yellow overlay that grows with charge ratio
    if (m_isCharging && m_config->m_chargedMaxChargeTime > 0.0f)
    {
        const float ratio   = m_chargeTime / m_config->m_chargedMaxChargeTime;
        const float clamped = ratio > 1.0f ? 1.0f : ratio;
        const int   fillEnd = static_cast<int>(clamped * static_cast<float>(arcSegs));

        for (int i = 0; i < fillEnd; ++i)
        {
            const float a0 = -halfRad + arcStep * static_cast<float>(i);
            const float a1 = a0 + arcStep;
            DebugDrawAPI::drawLine(posFlat + radialDir(a0) * range,
                                   posFlat + radialDir(a1) * range, colYellow);
        }
    }
}

void DeathChargedAttack::updateUI()
{
    DeathAbilityBase::updateUI();

    if (m_deathUI)
    {
        m_deathUI->updateChargedSlashUI(m_attackStateTimer, m_config->m_chargedAttackLockDuration);
    }
}

IMPLEMENT_SCRIPT(DeathChargedAttack)
