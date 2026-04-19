#include "pch.h"
#include "DeathDash.h"

#include "CharacterBase.h"
#include "EnemyDamageable.h"

static const ScriptFieldInfo LyrielDashFields[] =
{
    { "Dash Duration",        ScriptFieldType::Float, offsetof(DeathDash, m_dashDurationLyriel), { 0.0f, 1.0f,  0.01f } },
    { "Dash Distance",        ScriptFieldType::Float, offsetof(DeathDash, m_dashDistanceLyriel), { 0.0f, 10.0f, 0.1f  } },
    { "Dash Cooldown",        ScriptFieldType::Float, offsetof(DeathDash, m_dashCooldown),        { 0.0f, 5.0f,  0.01f } },
    { "Dash Hit Width",       ScriptFieldType::Float, offsetof(DeathDash, m_dashHitWidth),        { 0.1f, 5.0f,  0.05f } },
    { "Dash Damage",          ScriptFieldType::Float, offsetof(DeathDash, m_dashDamage),          { 0.0f, 100.0f, 1.0f } },
};

IMPLEMENT_SCRIPT_FIELDS(DeathDash, LyrielDashFields)

DeathDash::DeathDash(GameObject* owner): AbilityDash(owner)
{
}

void DeathDash::Start()
{
    m_dashDuration = m_dashDurationLyriel;
    m_dashDistance = m_dashDistanceLyriel;
    m_cooldown = m_dashCooldown;

    AbilityDash::Start();
}


void DeathDash::onDashStarted()
{
    Transform* t = GameObjectAPI::getTransform(getOwner());
    m_dashStartPosition = (t != nullptr) ? TransformAPI::getPosition(t) : Vector3::Zero;
    m_dashDamageDealt = false;
}

void DeathDash::onDashEnded()
{
    applyDashDamage();
}

void DeathDash::onDashUpdate(float dt)
{

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

        Script* script = GameObjectAPI::getScript(enemyObj, "EnemyDamageable");
        EnemyDamageable* damageable = dynamic_cast<EnemyDamageable*>(script);

        if (damageable != nullptr)
        {
            damageable->takeDamageEnemy(m_dashDamage, GameObjectAPI::getTransform(getOwner()));
        }
    }
}

IMPLEMENT_SCRIPT(DeathDash)