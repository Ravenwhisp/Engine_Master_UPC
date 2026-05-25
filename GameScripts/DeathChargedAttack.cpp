#include "pch.h"
#include "DeathChargedAttack.h"

#include "DeathCharacter.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"
#include "EnemyDamageable.h"
#include "EnemyShadowMark.h"
#include "BreakableDamageable.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(DeathChargedAttack, DeathAbilityBase,
    SERIALIZED_COMPONENT_REF(m_ChargedAttackUI, "Charged Attack UI", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_chargedAttackDamage, "Charged Attack Damage", 0.0f, 200.0f, 1.0f),
    SERIALIZED_FLOAT(m_arcRange, "Arc Range", 0.5f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_arcAngle, "Arc Angle", 10.0f, 360.0f, 5.0f),
    SERIALIZED_FLOAT(m_maxChargeTime, "Max Charge Time", 0.5f, 5.0f, 0.1f),
    SERIALIZED_FLOAT(m_minChargeTime, "Min Charge Time", 0.0f, 3.0f, 0.05f),
    SERIALIZED_FLOAT(m_attackLockDuration, "Attack Lock Duration", 0.05f, 2.0f, 0.05f),
    SERIALIZED_FLOAT(m_finalHitLockDuration, "Final Hit Lock Duration", 0.05f, 3.0f, 0.05f),
    SERIALIZED_FLOAT(m_chargedArcRange, "Charged Arc Range", 0.5f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_chargedArcAngle, "Charged Arc Angle", 10.0f, 360.0f, 5.0f)
)

DeathChargedAttack::DeathChargedAttack(GameObject* owner)
    : DeathAbilityBase(owner)
{
}

void DeathChargedAttack::Start()
{
    DeathAbilityBase::Start();
}

void DeathChargedAttack::Update()
{
    DeathAbilityBase::Update();

    // Release combo movement lock once the attack window is done and combo has ended
    if (m_movementLockedForCombo && !m_isCharging && m_attackStateTimer <= 0.0f)
    {
        if (m_deathCharacter->getComboStep() == 0)
        {
            releaseComboMoveLock();
        }
    }

    // Charging phase — accumulate time, sample right stick for aim, auto-fire at max
    if (m_isCharging)
    {
        m_chargeTime += Time::getDeltaTime();
        updateAimDirection();

        if (m_ChargedAttackUI.getReferencedComponent())
        {
            GameObjectAPI::setActive(m_ChargedAttackUI.getReferencedComponent()->getOwner(), true);

			const float yawRad = std::atan2(m_aimDirection.x, m_aimDirection.z);
			const float targetYawDeg = yawRad * (180.0f / 3.14159265f);

			TransformAPI::setPosition(m_ChargedAttackUI.getReferencedComponent(), TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner())));
			TransformAPI::setRotationEuler(m_ChargedAttackUI.getReferencedComponent(), Vector3(0.0f, targetYawDeg, 0.0f));
        }

        const bool maxReached = (m_chargeTime >= m_maxChargeTime);
        const bool released   = Input::isRightTriggerReleased(getPlayerIndex());

        if (maxReached || released)
        {
            fireAttack();
        }

        return;
    }

    else if (m_ChargedAttackUI.getReferencedComponent())
    {
        GameObjectAPI::setActive(m_ChargedAttackUI.getReferencedComponent()->getOwner(), false);
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

    Debug::log("[COMBO] R2 cargando  step=%d/3", m_deathCharacter->getComboStep() + 1);
}

void DeathChargedAttack::fireAttack()
{
    // Snap to the aim direction sampled during the hold, then deal damage in that direction
    snapFaceAimDirection();

    const int   comboStep   = m_deathCharacter->getComboStep();
    const bool  isMaxCharge = (m_chargeTime >= m_maxChargeTime);

    // Charged-mode shot: only valid as combo starter (step 0), needs min charge time
    const bool isChargedShot = (m_chargeTime >= m_minChargeTime) && (comboStep == 0);

    float damage;
    if (isChargedShot)
    {
        const float rawRatio    = m_chargeTime / m_maxChargeTime;
        const float chargeRatio = rawRatio > 1.0f ? 1.0f : rawRatio;
        damage = m_chargedAttackDamage * (1.0f + chargeRatio);

        if (isMaxCharge)
            Debug::log("[COMBO] R2 CARGA MAXIMA  step %d/3  dmg=%.1f", comboStep + 1, damage);
        else
            Debug::log("[COMBO] R2 CARGADO  step %d/3  ratio=%.0f%%  dmg=%.1f",
                comboStep + 1, (m_chargeTime / m_maxChargeTime) * 100.0f, damage);
    }
    else
    {
        damage = m_chargedAttackDamage;
        Debug::log("[COMBO] R2  step %d/3  dmg=%.1f", comboStep + 1, damage);
    }

    dealDamageInArc(damage, m_chargedArcRange, m_chargedArcAngle);

    // Max charge (auto-fired at full charge, always step 0) gets longer combo window
    const float window = (isChargedShot && isMaxCharge)
        ? m_deathCharacter->m_comboWindowMaxCharge
        : m_deathCharacter->m_comboWindowR2;
    m_deathCharacter->advanceCombo(true, window);

    const bool isLast = (comboStep >= 2);
    if (isLast)
        Debug::log("[COMBO] R2  step 3/3  COMPLETO — reset");

    m_chargeTime = 0.0f;
    m_isCharging = false;

    const float lockDuration = (comboStep >= 2) ? m_finalHitLockDuration : m_attackLockDuration;

    // Trigger attack animation and start the post-fire movement lock window
    beginAttackPresentation();
    beginAttackWindow(lockDuration);
}

void DeathChargedAttack::dealDamageInArc(float damage) const
{
    const Transform* myTransform = GameObjectAPI::getTransform(m_owner);
    if (myTransform == nullptr)
    {
        return;
    }

    Vector3 myPos = TransformAPI::getPosition(myTransform);
    Vector3 myForward = TransformAPI::getForward(myTransform);

    myForward.y = 0.0f;
    const float fwdLen = myForward.Length();
    if (fwdLen > 0.0001f)
    {
        myForward /= fwdLen;
    }

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    const float     halfAngleCos = cosf(m_arcAngle * 0.5f * k_degToRad);
    const float     arcRangeSq = m_arcRange * m_arcRange;

    const auto enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY);
	const auto breakables = SceneAPI::findAllGameObjectsByTag(Tag::BREAKABLE);
	auto targets = enemies;
	targets.insert(targets.end(), breakables.begin(), breakables.end());
    int scanned = 0;
    int hit = 0;

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

        Vector3 toEnemy = TransformAPI::getPosition(enemyTr) - myPos;
        toEnemy.y = 0.0f;

        const float distSq = toEnemy.LengthSquared();
        if (distSq > arcRangeSq)
        {
            continue;
        }

        if (m_arcAngle < 360.0f && distSq > 0.0001f)
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
            damageable->takeDamageEnemy(damage, GameObjectAPI::getTransform(getOwner()));
            Debug::log("[ARC] hit '%s'  dmg=%.1f  hp=%.1f/%.1f",
                GameObjectAPI::getName(target), damage,
                damageable->getCurrentHp(), damageable->getMaxHp());
        }
        hit++;

        EnemyShadowMark* shadowMark = GameObjectAPI::findScript<EnemyShadowMark>(target);
        if (shadowMark != nullptr)
        {
            shadowMark->notifyDeathHit();
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

void DeathChargedAttack::dealDamageInArc(float damage, float range, float angle) const //charged attack
{
    const float savedRange = m_arcRange;
    const float savedAngle = m_arcAngle;
    const_cast<DeathChargedAttack*>(this)->m_arcRange = range;
    const_cast<DeathChargedAttack*>(this)->m_arcAngle = angle;
    dealDamageInArc(damage);
    const_cast<DeathChargedAttack*>(this)->m_arcRange = savedRange;
    const_cast<DeathChargedAttack*>(this)->m_arcAngle = savedAngle;
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
    const Vector3   euler      = TransformAPI::getEulerDegrees(myTransform);
    TransformAPI::setRotationEuler(myTransform, Vector3(euler.x, yaw, euler.z));
}

void DeathChargedAttack::onAttackWindowUpdate()
{
    PlayerAnimationController* anim = m_character ? m_character->getAnimationController() : nullptr;
    if (anim != nullptr)
        anim->requestAttack();
}

void DeathChargedAttack::onAttackWindowFinished()
{
    // Between combo hits: keep movement locked while the combo is still alive
    if (m_movementLockedForCombo && m_deathCharacter != nullptr && m_deathCharacter->getComboStep() > 0)
    {
        PlayerState* ps = m_character ? m_character->getPlayerState() : nullptr;
        if (ps != nullptr && !ps->isDowned())
        {
            ps->setState(PlayerStateType::AttackRecovery);
        }
    }
}

void DeathChargedAttack::drawGizmo()
{
    if (m_deathCharacter == nullptr)
        return;

    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr)
        return;

    const Vector3 pos   = TransformAPI::getPosition(t);
    const float   range = m_chargedArcRange;

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
    const float   angle   = m_chargedArcAngle;
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
    if (m_isCharging && m_maxChargeTime > 0.0f)
    {
        const float ratio   = m_chargeTime / m_maxChargeTime;
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

IMPLEMENT_SCRIPT(DeathChargedAttack)
