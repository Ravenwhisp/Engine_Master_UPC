#include "pch.h"
#include "DeathDash.h"

#include "DeathCharacter.h"
#include "DeathSound.h"
#include "EnemyDamageable.h"
#include "EnemyShadowMark.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(DeathDash, AbilityDash,
    SERIALIZED_FLOAT(m_dashHitWidth, "Dash Hit Width", 0.1f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_dashDamage, "Dash Damage", 0.0f, 100.0f, 1.0f)
)

DeathDash::DeathDash(GameObject* owner): AbilityDash(owner)
{
}

void DeathDash::Start()
{
    AbilityDash::Start();

    if (m_character == nullptr)
    {
        Debug::log("[DeathDash] DeathCharacter not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }

    m_sound = GameObjectAPI::findScript<DeathSound>(getOwner());
}

void DeathDash::onDashStarted()
{
    Transform* t = GameObjectAPI::getTransform(getOwner());
    m_dashStartPosition = (t != nullptr) ? TransformAPI::getPosition(t) : Vector3::Zero;
    m_dashDamageDealt = false;
    m_dashImpactSoundPlayed = false;

    if (m_sound != nullptr)
    {
        m_sound->playDashWhoosh();
    }
}

void DeathDash::onDashEnded()
{
    applyDashDamage();
}

void DeathDash::onDashUpdate(float dt)
{
    // Play impact sound the first frame Death's path actually contains an enemy.
    // Damage is still applied at dash end by applyDashDamage; this only fixes timing.
    if (m_dashImpactSoundPlayed || m_sound == nullptr)
    {
        return;
    }

    if (anyEnemyInsideDashRectangle())
    {
        m_sound->playDashImpact();
        m_dashImpactSoundPlayed = true;
    }
}

bool DeathDash::anyEnemyInsideDashRectangle() const
{
    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);
    for (GameObject* enemy : enemies)
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
        if (isInsideDashRectangle(TransformAPI::getPosition(enemyTransform)))
        {
            return true;
        }
    }
    return false;
}


bool DeathDash::isInsideDashRectangle(const Vector3& point) const
{
    Transform* t = GameObjectAPI::getTransform(getOwner());
    Vector3    endPos = (t != nullptr) ? TransformAPI::getPosition(t) : Vector3::Zero;

    Vector3 start2D = { m_dashStartPosition.x, 0.0f, m_dashStartPosition.z };
    Vector3 end2D = { endPos.x,               0.0f, endPos.z };
    Vector3 pt2D = { point.x,                0.0f, point.z };

    Vector3 dashVec = end2D - start2D;
    float   length = dashVec.Length();

    if (length < 0.0001f)
    {
        return false;
    }

    Vector3 fwd = dashVec / length;

    Vector3 side = { -fwd.z, 0.0f, fwd.x };

    Vector3 toPoint = pt2D - start2D;
    float   longitudinal = toPoint.Dot(fwd);
    float   lateral = toPoint.Dot(side);

    return (longitudinal >= 0.0f && longitudinal <= length) && (lateral >= -m_dashHitWidth && lateral <= m_dashHitWidth);
}

void DeathDash::applyDashDamage()
{

    std::vector<GameObject*> allEnemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);

    bool anyMark = false;

    for (GameObject* enemyObj : allEnemies)
    {
        if (enemyObj == nullptr)
        {
            continue;
        }

        Transform* enemyTransform = GameObjectAPI::getTransform(enemyObj);
        if (enemyTransform == nullptr)
        {
            continue;
        }

        Vector3 enemyPos = TransformAPI::getPosition(enemyTransform);

        if (!isInsideDashRectangle(enemyPos))
        {
            continue;
        }

        EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(enemyObj);

        if (damageable != nullptr)
        {
            {
                EnemyHitContext ctx;
                ctx.damage = m_dashDamage;
                ctx.attacker = GameObjectAPI::getTransform(getOwner());
                ctx.attackType = EnemyAttackType::DeathDash;
                damageable->takeDamage(ctx);
            }

            EnemyShadowMark* shadowMark = GameObjectAPI::findScript<EnemyShadowMark>(enemyObj);
            if (shadowMark != nullptr)
            {
                shadowMark->notifyDeathHit();
                anyMark = true;
            }
        }
    }

    // playDashImpact is fired in onDashUpdate on first contact, not here.
    if (m_sound != nullptr && anyMark)
    {
        m_sound->playMarkApply();
    }
}

IMPLEMENT_SCRIPT(DeathDash)
