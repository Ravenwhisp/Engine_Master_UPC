#include "pch.h"
#include "DeathBasicAttack.h"
#include "DeathCharacter.h"
#include "PlayerTargetController.h"
#include "PlayerAnimationController.h"
#include "PlayerRotation.h"
#include "PlayerState.h"

#include <cmath>

static const ScriptFieldInfo DeathBasicAttackFields[] =
{
    { "Attack Lock Duration",       ScriptFieldType::Float, offsetof(DeathBasicAttack, m_attackLockDuration),   { 0.05f, 2.0f, 0.05f } },
    { "Final Hit Lock Duration",    ScriptFieldType::Float, offsetof(DeathBasicAttack, m_finalHitLockDuration), { 0.05f, 3.0f, 0.05f } },
};

IMPLEMENT_SCRIPT_FIELDS(DeathBasicAttack, DeathBasicAttackFields)

DeathBasicAttack::DeathBasicAttack(GameObject* owner)
    : DeathAbilityBase(owner)
{
}

void DeathBasicAttack::Start()
{
    DeathAbilityBase::Start();
}

void DeathBasicAttack::Update()
{
    DeathAbilityBase::Update();

    // Release movement lock when combo fully ends outside an attack window
    if (m_movementLockedForCombo && m_attackStateTimer <= 0.0f)
    {
        const bool comboActive = m_deathChar != nullptr && m_deathChar->getComboStep() > 0;
        if (!comboActive)
        {
            releaseComboMoveLock();
        }
    }

    // Block new input while the attack window is still running
    if (m_attackStateTimer > 0.0f)
    {
        return;
    }

    if (!Input::isRightShoulderJustPressed(getPlayerIndex()))
    {
        return;
    }

    tryAttack();
}

void DeathBasicAttack::tryAttack()
{
    if (m_character == nullptr || m_deathChar == nullptr)
    {
        return;
    }

    if (m_deathChar->isInComboCooldown())
    {
        return;
    }

    if (!canStartAbility())
    {
        return;
    }

    GameObject* target = m_character->getTargetController()
        ? m_character->getTargetController()->getCurrentTarget()
        : nullptr;

    setAbilityLocked(true);
    m_movementLockedForCombo = true;

    snapFaceTarget(target);
    m_attackFacingTarget = target;

    const int   comboStep = m_deathChar->getComboStep();
    const float damage    = m_deathChar->m_basicAttackDamage;

    m_deathChar->dealDamageBasicAttack(damage, target);
    m_deathChar->advanceCombo(false);

    const bool  isFinalHit  = (comboStep >= 2);
    const float lockDuration = isFinalHit ? m_finalHitLockDuration : m_attackLockDuration;

    beginAttackPresentation();
    beginAttackWindow(lockDuration);

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
}

void DeathBasicAttack::onAttackWindowFinished()
{
    m_attackFacingTarget = nullptr;

    // Between combo hits: keep movement locked while combo is still active
    if (m_movementLockedForCombo && m_deathChar != nullptr && m_deathChar->getComboStep() > 0)
    {
        PlayerState* ps = m_character ? m_character->getPlayerState() : nullptr;
        if (ps != nullptr)
        {
            ps->setState(PlayerStateType::Attacking);
        }
    }
}

void DeathBasicAttack::releaseComboMoveLock()
{
    m_movementLockedForCombo = false;

    // Another ability may still be holding the lock (e.g. charged attack window still active).
    // Leave PlayerState alone — that ability's finishAttackWindow will release it.
    if (m_character != nullptr && m_character->isUsingAbility())
        return;

    PlayerState* ps = m_character ? m_character->getPlayerState() : nullptr;
    if (ps != nullptr && ps->isAttacking())
        ps->setState(PlayerStateType::Normal);
}

void DeathBasicAttack::drawGizmo()
{
    if (m_deathChar == nullptr)
    {
        return;
    }

    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr)
    {
        return;
    }

    const Vector3 pos      = TransformAPI::getPosition(t);
    const Vector3 fwd      = TransformAPI::getForward(t);
    const float   range    = m_deathChar->m_basicAttackRange;
    const float   hitAngle = m_deathChar->m_basicAttackHitAngle;
    const float   fill     = m_deathChar->getComboFillRatio();

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    const float hitHalfRad     = hitAngle * 0.5f * k_degToRad;

    const Vector3 posFlat = { pos.x, pos.y, pos.z };

    const Vector3 colGrey   = { 0.35f, 0.35f, 0.35f };
    const Vector3 colPurple = { 0.9f,  0.0f,  0.9f  };
    const Vector3 colOrange = { 1.0f,  0.55f, 0.0f  };
    const Vector3 colFill   = m_deathChar->wasLastHitR2() ? colOrange : colPurple;

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
        DebugDrawAPI::drawLine(posFlat + radialDir(a0) * range,
                               posFlat + radialDir(a1) * range, colGrey);
    }

    // Hit zone: narrow arc with edge lines
    const Vector3 hitLeft  = radialDir(-hitHalfRad);
    const Vector3 hitRight = radialDir( hitHalfRad);
    DebugDrawAPI::drawLine(posFlat, posFlat + hitLeft  * range, colGrey);
    DebugDrawAPI::drawLine(posFlat, posFlat + hitRight * range, colGrey);

    const int   arcSegs = 12;
    const float arcStep = (hitAngle * k_degToRad) / static_cast<float>(arcSegs);
    for (int i = 0; i < arcSegs; ++i)
    {
        const float a0 = -hitHalfRad + arcStep * static_cast<float>(i);
        const float a1 = a0 + arcStep;
        DebugDrawAPI::drawLine(posFlat + radialDir(a0) * range,
                               posFlat + radialDir(a1) * range, colGrey);
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
            DebugDrawAPI::drawLine(posFlat, posFlat + radialDir(a) * range, colFill);
        }

        DebugDrawAPI::drawLine(posFlat, posFlat + hitLeft * range, colFill);
    }
}

void DeathBasicAttack::snapFaceTarget(GameObject* target)
{
    if (target == nullptr)
    {
        return;
    }

    Transform* myTransform     = GameObjectAPI::getTransform(getOwner());
    Transform* targetTransform = GameObjectAPI::getTransform(target);
    if (myTransform == nullptr || targetTransform == nullptr)
    {
        return;
    }

    const float rangeSq = m_deathChar->m_basicAttackRange * m_deathChar->m_basicAttackRange;

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
    const Vector3   euler      = TransformAPI::getEulerDegrees(myTransform);
    TransformAPI::setRotationEuler(myTransform, Vector3(euler.x, yaw, euler.z));
}

void DeathBasicAttack::faceTarget(GameObject* target)
{
    PlayerRotation* playerRotation = m_character ? m_character->getPlayerRotation() : nullptr;
    if (playerRotation == nullptr || target == nullptr)
    {
        return;
    }

    Transform* myTransform     = GameObjectAPI::getTransform(getOwner());
    Transform* targetTransform = GameObjectAPI::getTransform(target);

    Vector3 myPos     = TransformAPI::getGlobalPosition(myTransform);
    Vector3 targetPos = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 dir = targetPos - myPos;
    dir.y = 0.0f;

    const float rangeSq = m_deathChar->m_basicAttackRange * m_deathChar->m_basicAttackRange;
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

IMPLEMENT_SCRIPT(DeathBasicAttack)
