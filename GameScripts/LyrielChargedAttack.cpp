#include "pch.h"
#include "LyrielChargedAttack.h"

#include "LyrielCharacter.h"
#include "CharacterBase.h"
#include "ArrowPool.h"
#include "LyrielArrowProjectile.h"
#include "Damageable.h"
#include "PlayerState.h"

#include <cmath>

static const ScriptFieldInfo LyrielChargedAttackFields[] =
{
    { "Min Damage", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_minDamage), { 0.0f, 100.0f, 0.5f } },
    { "Max Damage", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_maxDamage), { 0.0f, 200.0f, 0.5f } },
    { "Max Charge Time", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_maxChargeTime), { 0.1f, 5.0f, 0.05f } },
    { "Min Attack Range", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_minAttackRange), { 0.0f, 50.0f, 0.1f } },
    { "Max Attack Range", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_maxAttackRange), { 0.0f, 50.0f, 0.1f } },
    { "Line Half Width", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_lineHalfWidth), { 0.1f, 10.0f, 0.05f } },
    { "Attack Cooldown", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_attackCooldown), { 0.0f, 10.0f, 0.05f } },
    { "Attack Lock Duration", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_attackLockDuration), { 0.0f, 2.0f, 0.01f } },
    { "Arrow Speed", ScriptFieldType::Float, offsetof(LyrielChargedAttack, m_arrowSpeed), { 0.0f, 100.0f, 0.5f } }
};

IMPLEMENT_SCRIPT_FIELDS(LyrielChargedAttack, LyrielChargedAttackFields)

LyrielChargedAttack::LyrielChargedAttack(GameObject* owner)
    : LyrielAbilityBase(owner)
{
}

void LyrielChargedAttack::Start()
{
    LyrielAbilityBase::Start();
    m_cooldown = m_attackCooldown;
}

void LyrielChargedAttack::Update()
{
    LyrielAbilityBase::Update();

    if (canStartCharge() && Input::isRightTriggerJustPressed(getPlayerIndex()))
    {
        beginCharge();
    }

    if (m_isCharging && Input::isRightTriggerPressed(getPlayerIndex()))
    {
        updateCharge();
    }

    if (m_isCharging && Input::isRightTriggerReleased(getPlayerIndex()))
    {
        releaseChargeAndShoot();
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

bool LyrielChargedAttack::canStartCharge() const
{
    return canStartAbility();
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
}

void LyrielChargedAttack::releaseChargeAndShoot()
{
    m_isCharging = false;

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
    applyChargedDamage(targets, damage);
    spawnChargedArrow(origin, forward);

    beginAttackPresentation();

    beginAttackWindow(m_attackLockDuration);
    m_cooldownTimer = m_cooldown;
    m_chargeTimer = 0.0f;

    Debug::log("[LyrielChargedAttack] Fired charged shot. Targets hit: %d Damage: %.2f",
        static_cast<int>(targets.size()), damage);
}

Vector3 LyrielChargedAttack::computeAimDirection() const
{
    const Vector2 lookAxis = Input::getLookAxis(getPlayerIndex());
    return Vector3(lookAxis.x, 0.0f, lookAxis.y);
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

    Vector3 flatForward = forward;
    flatForward.y = 0.0f;

    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatForward.Normalize();

    const float currentRange = computeChargedRange();
    const float lineHalfWidthSq = m_lineHalfWidth * m_lineHalfWidth;

    for (GameObject* enemy : allEnemies)
    {
        if (enemy == nullptr)
        {
            continue;
        }

        Transform* enemyTransform = GameObjectAPI::getTransform(enemy);
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
            outTargets.push_back(enemy);
        }
    }
}

void LyrielChargedAttack::applyChargedDamage(const std::vector<GameObject*>& targets, float damage)
{
    for (GameObject* target : targets)
    {
        if (target == nullptr)
        {
            continue;
        }

        Script* script = GameObjectAPI::getScript(target, "Damageable");
        Damageable* damageable = dynamic_cast<Damageable*>(script);

        if (damageable != nullptr)
        {
            damageable->takeDamage(damage);
        }
    }
}

void LyrielChargedAttack::spawnChargedArrow(const Vector3& origin, const Vector3& forward)
{
    if (m_lyriel == nullptr)
    {
        return;
    }

    ArrowPool* arrowPool = m_lyriel->getArrowPool();
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