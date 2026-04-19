#include "pch.h"
#include "LyrielArrowVolley.h"

#include "LyrielCharacter.h"
#include "CharacterBase.h"
#include "ArrowPool.h"
#include "LyrielArrowProjectile.h"
#include "Damageable.h"
#include "PlayerState.h"

#include <cmath>

static const ScriptFieldInfo LyrielArrowVolleyFields[] =
{
    { "Volley Damage", ScriptFieldType::Float, offsetof(LyrielArrowVolley, m_volleyDamage), { 0.0f, 100.0f, 0.5f } },
    { "Volley Cooldown", ScriptFieldType::Float, offsetof(LyrielArrowVolley, m_volleyCooldown), { 0.0f, 20.0f, 0.1f } },
    { "Volley Range", ScriptFieldType::Float, offsetof(LyrielArrowVolley, m_volleyRange), { 0.0f, 50.0f, 0.1f } },
    { "Cone Angle Degrees", ScriptFieldType::Float, offsetof(LyrielArrowVolley, m_coneAngleDegrees), { 1.0f, 180.0f, 1.0f } },
    { "Num Visual Arrows", ScriptFieldType::Int, offsetof(LyrielArrowVolley, m_numVisualArrows), { 1.0f, 20.0f, 1.0f } },
    { "Arrow Speed", ScriptFieldType::Float, offsetof(LyrielArrowVolley, m_arrowSpeed), { 0.0f, 100.0f, 0.5f } },
    { "Attack Lock Duration", ScriptFieldType::Float, offsetof(LyrielArrowVolley, m_attackLockDuration), { 0.0f, 2.0f, 0.01f } }
};

IMPLEMENT_SCRIPT_FIELDS(LyrielArrowVolley, LyrielArrowVolleyFields)

LyrielArrowVolley::LyrielArrowVolley(GameObject* owner)
    : LyrielAbilityBase(owner)
{
}

void LyrielArrowVolley::Start()
{
    LyrielAbilityBase::Start();
    m_cooldown = m_volleyCooldown;
}

void LyrielArrowVolley::Update()
{
    LyrielAbilityBase::Update();

    if (canStartAim() && Input::isLeftTriggerJustPressed(getPlayerIndex()))
    {
        beginAim();
    }

    if (m_isAiming && Input::isLeftTriggerPressed(getPlayerIndex()))
    {
        updateAim();
    }

    if (m_isAiming && Input::isLeftTriggerReleased(getPlayerIndex()))
    {
        releaseAimAndCast();
    }
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

bool LyrielArrowVolley::canStartAim() const
{
    return canStartAbility();
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
}

void LyrielArrowVolley::updateAim()
{
    Vector3 aimDirection = computeAimDirection();
    if (isAimStickValid(aimDirection))
    {
        m_currentAimDirection = aimDirection;
    }
}

void LyrielArrowVolley::releaseAimAndCast()
{
    m_isAiming = false;

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

    std::vector<GameObject*> targets;
    collectEnemiesInCone(origin, forward, targets);
    applyVolleyDamage(targets);
    spawnVolleyArrows(origin, forward);

    beginAttackPresentation();

    beginAttackWindow(m_attackLockDuration);
    m_cooldownTimer = m_cooldown;

    Debug::log("[LyrielArrowVolley] Cast Arrow Volley. Targets hit: %d", static_cast<int>(targets.size()));
}

Vector3 LyrielArrowVolley::computeAimDirection() const
{
    const Vector2 lookAxis = Input::getLookAxis(getPlayerIndex());
    return Vector3(lookAxis.x, 0.0f, lookAxis.y);
}

bool LyrielArrowVolley::isAimStickValid(const Vector3& direction) const
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;
    return flatDirection.LengthSquared() > 0.0001f;
}

void LyrielArrowVolley::collectEnemiesInCone(const Vector3& origin, const Vector3& forward, std::vector<GameObject*>& outTargets)
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

    const float halfAngleRadians = DirectX::XMConvertToRadians(m_coneAngleDegrees * 0.5f);
    const float minDot = std::cos(halfAngleRadians);

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
        Vector3 toEnemy = enemyPosition - origin;
        toEnemy.y = 0.0f;

        const float distanceSq = toEnemy.LengthSquared();
        if (distanceSq <= 0.0001f)
        {
            continue;
        }

        if (distanceSq > (m_volleyRange * m_volleyRange))
        {
            continue;
        }

        toEnemy.Normalize();

        const float dot = flatForward.Dot(toEnemy);
        if (dot >= minDot)
        {
            outTargets.push_back(enemy);
        }
    }
}

void LyrielArrowVolley::applyVolleyDamage(const std::vector<GameObject*>& targets)
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
            damageable->takeDamage(m_volleyDamage);
        }
    }
}

void LyrielArrowVolley::spawnVolleyArrows(const Vector3& origin, const Vector3& forward)
{
    if (m_lyriel == nullptr || m_numVisualArrows <= 0)
    {
        return;
    }

    ArrowPool* arrowPool = m_lyriel->getArrowPool();
    if (arrowPool == nullptr)
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

    const float totalAngle = m_coneAngleDegrees;

    float lifetime = 0.0f;
    if (m_arrowSpeed > 0.0001f)
    {
        lifetime = m_volleyRange / m_arrowSpeed;
    }

    for (int i = 0; i < m_numVisualArrows; ++i)
    {
        LyrielArrowProjectile* arrow = arrowPool->acquireArrow();
        if (arrow == nullptr)
        {
            Debug::log("[LyrielArrowVolley] No available arrow in pool for visual arrow %d.", i);
            return;
        }

        float t = 0.5f;
        if (m_numVisualArrows > 1)
        {
            t = static_cast<float>(i) / static_cast<float>(m_numVisualArrows - 1);
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

        arrow->launch(origin, dir, m_arrowSpeed, lifetime, nullptr, 0.0f);
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

    const float halfAngleRad = DirectX::XMConvertToRadians(m_coneAngleDegrees * 0.5f);
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

    DebugDrawAPI::drawLine(origin, origin + leftDir * m_volleyRange, previewColor, 0, true);
    DebugDrawAPI::drawLine(origin, origin + rightDir * m_volleyRange, previewColor, 0, true);

    Vector3 previousPoint = origin + leftDir * m_volleyRange;

    for (int i = 1; i <= arcSteps; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(arcSteps);
        const float angle = -halfAngleRad + (2.0f * halfAngleRad * t);

        Vector3 arcDir = rotateXZ(flatForward, angle);
        arcDir.Normalize();

        Vector3 currentPoint = origin + arcDir * m_volleyRange;

        DebugDrawAPI::drawLine(previousPoint, currentPoint, previewColor, 0, true);
        previousPoint = currentPoint;
    }
}

IMPLEMENT_SCRIPT(LyrielArrowVolley)