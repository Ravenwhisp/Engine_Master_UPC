#include "pch.h"
#include "LyrielChargedAttack.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "CharacterBase.h"
#include "ArrowPool.h"
#include "LyrielArrowProjectile.h"
#include "EnemyDamageable.h"
#include "EnemyShadowMark.h"
#include "PlayerState.h"
#include "BreakableDamageable.h"

#include <cmath>

static const float PI = 3.1415926535897931f;

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LyrielChargedAttack, LyrielAbilityBase,
    SERIALIZED_COMPONENT_REF(m_ChargedAttackUI, "Charged Attack UI", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_minDamage, "Min Damage", 0.0f, 100.0f, 0.5f),
    SERIALIZED_FLOAT(m_maxDamage, "Max Damage", 0.0f, 200.0f, 0.5f),
    SERIALIZED_FLOAT(m_maxChargeTime, "Max Charge Time", 0.1f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_minAttackRange, "Min Attack Range", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(m_maxAttackRange, "Max Attack Range", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(m_lineHalfWidth, "Line Half Width", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_attackLockDuration, "Attack Lock Duration", 0.0f, 2.0f, 0.01f),
    SERIALIZED_FLOAT(m_arrowSpeed, "Arrow Speed", 0.0f, 100.0f, 0.5f)
)

LyrielChargedAttack::LyrielChargedAttack(GameObject* owner)
    : LyrielAbilityBase(owner)
{
}

void LyrielChargedAttack::Start()
{
    LyrielAbilityBase::Start();
}

void LyrielChargedAttack::Update()
{
    LyrielAbilityBase::Update();

    if (m_isCharging)
    {
        if (!Input::isRightTriggerReleased(getPlayerIndex()))
        {
            updateCharge();
        }
        else
        {
            releaseChargeAndShoot();
        }
	}
}

void LyrielChargedAttack::drawGizmo()
{
    if (!m_isCharging)
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
    drawChargePreview(origin, previewDirection);
}

void LyrielChargedAttack::onAttackWindowUpdate()
{
    if (m_attackFacingDirection.LengthSquared() > 0.0001f)
    {
        faceDirection(m_attackFacingDirection);
    }
}

void LyrielChargedAttack::onAttackWindowFinished()
{
    m_attackFacingDirection = Vector3::Zero;
}

void LyrielChargedAttack::startAbility()
{
    beginCharge();
}

bool LyrielChargedAttack::canStartCharge() const
{
    return canStartAbility(); //borrar
}

bool LyrielChargedAttack::canShoot() const
{
    return m_character != nullptr && !m_character->isDowned();
}

void LyrielChargedAttack::beginCharge()
{
    m_isCharging = true;
    setAbilityLocked(true);

    m_chargeTimer = 0.0f;
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
    if (m_ChargedAttackUI.getReferencedComponent())
    {
        GameObjectAPI::setActive(m_ChargedAttackUI.getReferencedComponent()->getOwner(), true);
    }

    LyrielSound* sound = m_lyrielCharacter != nullptr ? m_lyrielCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->startChargedTenseLoop();
    }
}

void LyrielChargedAttack::updateCharge()
{
    m_chargeTimer += Time::getDeltaTime();
    if (m_chargeTimer > m_maxChargeTime)
    {
        m_chargeTimer = m_maxChargeTime;
    }

    Vector3 aimDirection = computeAimDirection();
    if (isAimStickValid(aimDirection))
    {
        m_currentAimDirection = aimDirection;
    }

    if (m_ChargedAttackUI.getReferencedComponent())
    {
		const Vector3 origin = TransformAPI::getGlobalPosition(GameObjectAPI::getTransform(getOwner()));

        const float yawRad = std::atan2(m_currentAimDirection.x, m_currentAimDirection.z);
        const float targetYawDeg = yawRad * (180.0f / PI);

		const float timerRatio = m_chargeTimer / m_maxChargeTime;
        const float range = m_minAttackRange + timerRatio * (m_maxAttackRange - m_minAttackRange);

        TransformAPI::setPosition(m_ChargedAttackUI.getReferencedComponent(), origin);
        TransformAPI::setRotationEuler(m_ChargedAttackUI.getReferencedComponent(), Vector3(0.0f, targetYawDeg, 0.0f));
        TransformAPI::setScale(m_ChargedAttackUI.getReferencedComponent(), Vector3(1.0f, 1.0f, range));
    }
}

void LyrielChargedAttack::releaseChargeAndShoot()
{
    m_isCharging = false;

    LyrielSound* sound = m_lyrielCharacter != nullptr ? m_lyrielCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->stopChargedTenseLoop();
    }

    if (m_ChargedAttackUI.getReferencedComponent())
    {
        GameObjectAPI::setActive(m_ChargedAttackUI.getReferencedComponent()->getOwner(), false);
    }

    if (!canShoot())
    {
        setAbilityLocked(false);
        m_chargeTimer = 0.0f;
        return;
    }

    Transform* spawnTransform = findArrowSpawnTransform();
    if (spawnTransform == nullptr)
    {
        setAbilityLocked(false);
        m_chargeTimer = 0.0f;
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
        m_chargeTimer = 0.0f;
        return;
    }

    const float damage = computeChargedDamage();

    m_attackFacingDirection = forward;
    faceDirection(forward);

    std::vector<GameObject*> targets;
    collectEnemiesInLine(origin, forward, targets);
    const bool anyMarkExploited = applyChargedDamage(targets, damage);
    spawnChargedArrow(origin, forward);

    if (sound != nullptr)
    {
        sound->playChargedRelease();
        if (!targets.empty())
        {
            sound->playChargedImpact();
        }
        if (anyMarkExploited)
        {
            sound->playMarkExploit();
        }
    }

    beginAttackPresentation();

    beginAttackWindow(m_attackLockDuration);
    startCooldown();
    m_chargeTimer = 0.0f;

    Debug::log("[LyrielChargedAttack] Fired charged shot. Targets hit: %d Damage: %.2f",
        static_cast<int>(targets.size()), damage);
}

Vector3 LyrielChargedAttack::computeAimDirection() const
{
    return computeCameraRelativeAimDirection();
}

float LyrielChargedAttack::computeChargedDamage() const
{
    float chargeRatio = 0.0f;

    if (m_maxChargeTime > 0.0001f)
    {
        chargeRatio = m_chargeTimer / m_maxChargeTime;
    }

    if (chargeRatio < 0.0f)
    {
        chargeRatio = 0.0f;
    }

    if (chargeRatio > 1.0f)
    {
        chargeRatio = 1.0f;
    }

    return m_minDamage + (m_maxDamage - m_minDamage) * chargeRatio;
}

float LyrielChargedAttack::computeChargedRange() const
{
    float chargeRatio = 0.0f;

    if (m_maxChargeTime > 0.0001f)
    {
        chargeRatio = m_chargeTimer / m_maxChargeTime;
    }

    if (chargeRatio < 0.0f)
    {
        chargeRatio = 0.0f;
    }

    if (chargeRatio > 1.0f)
    {
        chargeRatio = 1.0f;
    }

    return m_minAttackRange + (m_maxAttackRange - m_minAttackRange) * chargeRatio;
}

bool LyrielChargedAttack::isAimStickValid(const Vector3& direction) const
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;
    return flatDirection.LengthSquared() > 0.0001f;
}

void LyrielChargedAttack::collectEnemiesInLine(const Vector3& origin, const Vector3& forward, std::vector<GameObject*>& outTargets)
{
    outTargets.clear();

    std::vector<GameObject*> allEnemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);
	std::vector<GameObject*> breakables = SceneAPI::findAllGameObjectsByTag(Tag::BREAKABLE, true);

	std::vector<GameObject*> potentialTargets = allEnemies;
	potentialTargets.insert(potentialTargets.end(), breakables.begin(), breakables.end());
    //de momento la mejor "manera" que veo es esta, habra que hacer refactor o cambios o algo

    Vector3 flatForward = forward;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    const float currentRange = computeChargedRange();
    const float lineHalfWidthSq = m_lineHalfWidth * m_lineHalfWidth;

    for (GameObject* target : potentialTargets)
    {
        if (target == nullptr)
        {
            continue;
        }

        Transform* enemyTransform = GameObjectAPI::getTransform(target);
        if (enemyTransform == nullptr)
        {
            continue;
        }

        Vector3 enemyPosition = TransformAPI::getGlobalPosition(enemyTransform);
        enemyPosition.y = origin.y;

        Vector3 toEnemy = enemyPosition - origin;
        toEnemy.y = 0.0f;

        const float forwardDistance = toEnemy.Dot(flatForward);
        if (forwardDistance < 0.0f)
        {
            continue;
        }

        if (forwardDistance > currentRange)
        {
            continue;
        }

        Vector3 closestPoint = origin + flatForward * forwardDistance;
        Vector3 lateralOffset = enemyPosition - closestPoint;
        lateralOffset.y = 0.0f;

        if (lateralOffset.LengthSquared() <= lineHalfWidthSq)
        {
            outTargets.push_back(target);
        }
    }
}

bool LyrielChargedAttack::applyChargedDamage(const std::vector<GameObject*>& targets, float damage)
{
    bool anyMarkExploited = false;

    for (GameObject* target : targets)
    {
        if (target == nullptr)
        {
            continue;
        }

        EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(target);

        if (damageable != nullptr)
        {
            {
                EnemyHitContext ctx;
                ctx.damage = damage;
                ctx.attacker = GameObjectAPI::getTransform(getOwner());
                ctx.attackType = EnemyAttackType::LyrielCharged;
                damageable->takeDamage(ctx);
            }

            EnemyShadowMark* mark = GameObjectAPI::findScript<EnemyShadowMark>(target);
            if (mark != nullptr && mark->isExploitable())
            {
                mark->exploit();
                anyMarkExploited = true;
                if (m_lyrielCharacter != nullptr)
                    m_lyrielCharacter->onMarkExploited();
            }
            continue;
        }
        BreakableDamageable* breakableDamageable = GameObjectAPI::findScript<BreakableDamageable>(target);
        if (breakableDamageable != nullptr)
        {
            breakableDamageable->takeDamage(damage);
        }
    }

    return anyMarkExploited;
}

void LyrielChargedAttack::spawnChargedArrow(const Vector3& origin, const Vector3& forward)
{
    if (m_lyrielCharacter == nullptr)
    {
        return;
    }

    ArrowPool* arrowPool = m_lyrielCharacter->getArrowPool();
    if (arrowPool == nullptr)
    {
        return;
    }

    LyrielArrowProjectile* arrow = arrowPool->acquireArrow();
    if (arrow == nullptr)
    {
        Debug::log("[LyrielChargedAttack] No available arrow in pool for charged shot visual.");
        return;
    }

    Vector3 flatForward = forward;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    const float range = computeChargedRange();

    float lifetime = 0.0f;
    if (m_arrowSpeed > 0.0001f)
    {
        lifetime = range / m_arrowSpeed;
    }

    arrow->launch(origin, flatForward, m_arrowSpeed, lifetime, nullptr, 0.0f);
}

void LyrielChargedAttack::drawChargePreview(const Vector3& origin, const Vector3& forward) const
{
    Vector3 flatForward = forward;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    const float previewRange = computeChargedRange();

    Vector3 right(-flatForward.z, 0.0f, flatForward.x);
    if (right.LengthSquared() <= 0.0001f)
    {
        return;
    }
    right.Normalize();

    const Vector3 previewColor(0.2f, 1.0f, 1.0f);

    const Vector3 leftStart = origin - right * m_lineHalfWidth;
    const Vector3 rightStart = origin + right * m_lineHalfWidth;

    const Vector3 leftEnd = leftStart + flatForward * previewRange;
    const Vector3 rightEnd = rightStart + flatForward * previewRange;

    DebugDrawAPI::drawLine(leftStart, leftEnd, previewColor, 0, true);
    DebugDrawAPI::drawLine(rightStart, rightEnd, previewColor, 0, true);
    DebugDrawAPI::drawLine(leftEnd, rightEnd, previewColor, 0, true);
}

IMPLEMENT_SCRIPT(LyrielChargedAttack)
