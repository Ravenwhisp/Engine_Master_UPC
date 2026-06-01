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

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(DeathBasicAttack, DeathAbilityBase,

    SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 200.0f, 1.0f),
    SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.5f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_basicAttackHitAngle, "Basic Attack Hit Angle", 5.0f, 180.0f, 5.0f),
    SERIALIZED_FLOAT(m_attackLockDuration, "Attack Lock Duration", 0.05f, 2.0f, 0.05f),
    SERIALIZED_FLOAT(m_finalHitLockDuration, "Final Hit Lock Duration", 0.05f, 3.0f, 0.05f)
)

DeathBasicAttack::DeathBasicAttack(GameObject* owner)
    : DeathAbilityBase(owner)
{
}

void DeathBasicAttack::Start()
{
    DeathAbilityBase::Start();

    setupUI();
}

void DeathBasicAttack::Update()
{
	DeathAbilityBase::Update();
    // Release movement lock when combo fully ends outside an attack window
    if (m_movementLockedForCombo && m_attackStateTimer <= 0.0f)
    {
        const bool comboActive = m_deathCharacter != nullptr && m_deathCharacter->getComboStep() > 0;
        if (!comboActive)
        {
            releaseComboMoveLock();
        }
    }

    updateUI();

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
    m_deathCharacter->advanceCombo(false);

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
    if (m_movementLockedForCombo && m_deathCharacter != nullptr && m_deathCharacter->getComboStep() > 0)
    {
        PlayerState* ps = m_character ? m_character->getPlayerState() : nullptr;
        if (ps != nullptr && !ps->isDowned())
        {
            ps->setState(PlayerStateType::AttackRecovery);
        }
    }
}

void DeathBasicAttack::dealDamageToTarget(GameObject* target) const
{
    const Transform* myTransform = GameObjectAPI::getTransform(m_owner);
    if (myTransform == nullptr)
    {
        return;
    }

    const Vector3 myPos = TransformAPI::getPosition(myTransform);
    Vector3 myForward = TransformAPI::getForward(myTransform);
    myForward.y = 0.0f;
    const float fwdLen = myForward.Length();
    if (fwdLen > 0.0001f)
    {
        myForward /= fwdLen;
    }

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    const float halfHitCos = cosf(m_basicAttackHitAngle * 0.5f * k_degToRad);
    const float rangeSq = m_basicAttackRange * m_basicAttackRange;

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
            Vector3 toE = TransformAPI::getPosition(eTr) - myPos;
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
                breakableDamageable->takeDamage(m_basicAttackDamage);
                if (sound != nullptr)
                {
                    sound->playLightImpact();
                }
                return;
            }

            {
                EnemyHitContext ctx;
                ctx.damage = m_basicAttackDamage;
                ctx.attacker = GameObjectAPI::getTransform(getOwner());
                ctx.attackType = EnemyAttackType::DeathBasic;
                damageable->takeDamage(ctx);
            }

            Debug::log("[BASIC] hit '%s'  dmg=%.1f  hp=%.1f/%.1f",
                GameObjectAPI::getName(enemy),
                m_basicAttackDamage,
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
        Vector3 toE = TransformAPI::getPosition(eTr) - myPos;
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

    const Vector3 pos      = TransformAPI::getPosition(t);
    const Vector3 fwd      = TransformAPI::getForward(t);
    const float   fill     = m_deathCharacter->getComboFillRatio();

    constexpr float k_degToRad = 3.14159265f / 180.0f;
    const float hitHalfRad     = m_basicAttackHitAngle * 0.5f * k_degToRad;

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
        DebugDrawAPI::drawLine(posFlat + radialDir(a0) * m_basicAttackRange,
                               posFlat + radialDir(a1) * m_basicAttackRange, colGrey);
    }

    // Hit zone: narrow arc with edge lines
    const Vector3 hitLeft  = radialDir(-hitHalfRad);
    const Vector3 hitRight = radialDir( hitHalfRad);
    DebugDrawAPI::drawLine(posFlat, posFlat + hitLeft  * m_basicAttackRange, colGrey);
    DebugDrawAPI::drawLine(posFlat, posFlat + hitRight * m_basicAttackRange, colGrey);

    const int   arcSegs = 12;
    const float arcStep = (m_basicAttackHitAngle * k_degToRad) / static_cast<float>(arcSegs);
    for (int i = 0; i < arcSegs; ++i)
    {
        const float a0 = -hitHalfRad + arcStep * static_cast<float>(i);
        const float a1 = a0 + arcStep;
        DebugDrawAPI::drawLine(posFlat + radialDir(a0) * m_basicAttackRange,
                               posFlat + radialDir(a1) * m_basicAttackRange, colGrey);
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
            DebugDrawAPI::drawLine(posFlat, posFlat + radialDir(a) * m_basicAttackRange, colFill);
        }

        DebugDrawAPI::drawLine(posFlat, posFlat + hitLeft * m_basicAttackRange, colFill);
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

    const float rangeSq = m_basicAttackRange * m_basicAttackRange;

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

    const float rangeSq = m_basicAttackRange * m_basicAttackRange;
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

void DeathBasicAttack::setupUI()
{
    Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t)
    {
        m_deathSlashUITransform = TransformAPI::findChildByName(t, "DeathSlashUI");
        if (m_deathSlashUITransform)
        {
            GameObjectAPI::setActive(m_deathSlashUITransform->getOwner(), false);
            m_deathSlashUISlider = static_cast<UISlider*>(GameObjectAPI::getComponent(m_deathSlashUITransform->getOwner(), ComponentType::UISLIDER));
            if (m_deathSlashUISlider)
            {
                SliderAPI::setFillAmount(m_deathSlashUISlider, 0.0f);
            }
        }
    }

    if (!m_deathSlashUITransform)
    {
        Debug::warn("DeathBasicAttack on '%s' could not find DeathSlashUI child for attack UI.", GameObjectAPI::getName(getOwner()));
    }
    else if (!m_deathSlashUISlider)
    {
        Debug::warn("DeathBasicAttack on '%s' could not find UISlider on DeathSlashUI for attack UI.", GameObjectAPI::getName(getOwner()));
    }
}

void DeathBasicAttack::updateUI()
{
    AbilityBase::updateUI();

    if (m_deathSlashUITransform == nullptr || m_deathSlashUISlider == nullptr)
    {
        return;
    }

	Debug::log("[UI] attack window timer: %.2f / %.2f", m_attackStateTimer, m_attackLockDuration);

    const bool showUI = m_attackStateTimer > 0.0f;
    GameObjectAPI::setActive(m_deathSlashUITransform->getOwner(), showUI);
    if (showUI)
    {
        const float t = 1.0f - (m_attackStateTimer / m_attackLockDuration);
        SliderAPI::setFillOrigin(m_deathSlashUISlider, t < 0.5f ? FillOrigin::Radial180BottomCCW : FillOrigin::Radial180Bottom);
        const float fillAmount = MathAPI::pingPong(t);
        const float easedFill = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, fillAmount);
        SliderAPI::setFillAmount(m_deathSlashUISlider, fillAmount);
    }
}

IMPLEMENT_SCRIPT(DeathBasicAttack)
