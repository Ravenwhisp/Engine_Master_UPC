#include "pch.h"
#include "DeathChargedAttack.h"
#include "DeathCharacter.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"

#include <cmath>

static const ScriptFieldInfo DeathChargedAttackFields[] =
{
    { "Min Charge Time",         ScriptFieldType::Float, offsetof(DeathChargedAttack, m_minChargeTime),        { 0.0f,  3.0f, 0.05f } },
    { "Attack Lock Duration",    ScriptFieldType::Float, offsetof(DeathChargedAttack, m_attackLockDuration),   { 0.05f, 2.0f, 0.05f } },
    { "Final Hit Lock Duration", ScriptFieldType::Float, offsetof(DeathChargedAttack, m_finalHitLockDuration), { 0.05f, 3.0f, 0.05f } },
    { "Charged Arc Range",       ScriptFieldType::Float, offsetof(DeathChargedAttack, m_chargedArcRange),      { 0.5f, 10.0f, 0.1f  } },
    { "Charged Arc Angle",       ScriptFieldType::Float, offsetof(DeathChargedAttack, m_chargedArcAngle),      { 10.0f, 360.0f, 5.0f } },
};

IMPLEMENT_SCRIPT_FIELDS(DeathChargedAttack, DeathChargedAttackFields)

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

    if (m_character == nullptr || m_deathChar == nullptr)
        return;

    // Release combo movement lock once the attack window is done and combo has ended
    if (m_movementLockedForCombo && !m_isCharging && m_attackStateTimer <= 0.0f)
    {
        if (m_deathChar->getComboStep() == 0)
            releaseComboMoveLock();
    }

    // Attack window running — base class handles the timer, no new input accepted
    if (m_attackStateTimer > 0.0f)
        return;

    // Charging phase — accumulate time, sample right stick for aim, auto-fire at max
    if (m_isCharging)
    {
        m_chargeTime += Time::getDeltaTime();
        updateAimDirection();

        const bool maxReached = (m_chargeTime >= m_deathChar->m_maxChargeTime);
        const bool released   = Input::isRightTriggerReleased(getPlayerIndex());

        if (maxReached || released)
            fireAttack();

        return;
    }

    // Wait for R2 press
    if (!Input::isRightTriggerJustPressed(getPlayerIndex()))
        return;

    if (m_deathChar->isInComboCooldown())
    {
        Debug::log("[R2] bloqueado — cooldown post-combo");
        return;
    }

    if (!canStartAbility())
        return;

    if (!m_deathChar->canUseR2InCombo())
    {
        Debug::log("[R2] bloqueado — 2 R2 consecutivos ya usados, usa R1 primero");
        return;
    }

    startCharging();
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
        ps->setState(PlayerStateType::Attacking);

    Debug::log("[COMBO] R2 cargando  step=%d/3", m_deathChar->getComboStep() + 1);
}

void DeathChargedAttack::fireAttack()
{
    // Snap to the aim direction sampled during the hold, then deal damage in that direction
    snapFaceAimDirection();

    const int   comboStep   = m_deathChar->getComboStep();
    const bool  isMaxCharge = (m_chargeTime >= m_deathChar->m_maxChargeTime);

    // Charged-mode shot: only valid as combo starter (step 0), needs min charge time
    const bool isChargedShot = (m_chargeTime >= m_minChargeTime) && (comboStep == 0);

    float damage;
    if (isChargedShot)
    {
        const float rawRatio    = m_chargeTime / m_deathChar->m_maxChargeTime;
        const float chargeRatio = rawRatio > 1.0f ? 1.0f : rawRatio;
        damage = m_deathChar->m_chargedAttackDamage * (1.0f + chargeRatio);

        if (isMaxCharge)
            Debug::log("[COMBO] R2 CARGA MAXIMA  step %d/3  dmg=%.1f", comboStep + 1, damage);
        else
            Debug::log("[COMBO] R2 CARGADO  step %d/3  ratio=%.0f%%  dmg=%.1f",
                comboStep + 1, (m_chargeTime / m_deathChar->m_maxChargeTime) * 100.0f, damage);
    }
    else
    {
        damage = m_deathChar->m_chargedAttackDamage;
        Debug::log("[COMBO] R2  step %d/3  dmg=%.1f", comboStep + 1, damage);
    }

    m_deathChar->dealDamageInArc(damage, m_chargedArcRange, m_chargedArcAngle);

    // Max charge (auto-fired at full charge, always step 0) gets longer combo window
    const float window = (isChargedShot && isMaxCharge)
        ? m_deathChar->m_comboWindowMaxCharge
        : m_deathChar->m_comboWindowR2;
    m_deathChar->advanceCombo(true, window);

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

void DeathChargedAttack::updateAimDirection()
{
    const Vector2 lookAxis = Input::getLookAxis(getPlayerIndex());
    const float   magSq    = lookAxis.x * lookAxis.x + lookAxis.y * lookAxis.y;

    // Only update if the stick is past the deadzone (~0.3 magnitude)
    if (magSq >= 0.09f)
        m_aimDirection = Vector3(lookAxis.x, 0.0f, lookAxis.y);
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
    if (m_movementLockedForCombo && m_deathChar != nullptr && m_deathChar->getComboStep() > 0)
    {
        PlayerState* ps = m_character ? m_character->getPlayerState() : nullptr;
        if (ps != nullptr)
            ps->setState(PlayerStateType::Attacking);
    }
}

void DeathChargedAttack::releaseComboMoveLock()
{
    m_movementLockedForCombo = false;

    // Another ability may still be holding the lock (e.g. basic attack window still active).
    // Leave PlayerState alone — that ability's finishAttackWindow will release it.
    if (m_character != nullptr && m_character->isUsingAbility())
        return;

    PlayerState* ps = m_character ? m_character->getPlayerState() : nullptr;
    if (ps != nullptr && ps->isAttacking())
        ps->setState(PlayerStateType::Normal);
}

void DeathChargedAttack::drawGizmo()
{
    if (m_deathChar == nullptr)
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
    if (m_isCharging && m_deathChar->m_maxChargeTime > 0.0f)
    {
        const float ratio   = m_chargeTime / m_deathChar->m_maxChargeTime;
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
