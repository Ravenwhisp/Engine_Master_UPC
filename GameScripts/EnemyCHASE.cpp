#include "pch.h"
#include "EnemyCHASE.h"

static const ScriptFieldInfo CHASEFields[] =
{
    { "Speed", ScriptFieldType::Float, offsetof(EnemyCHASE, m_speed), { 0.0f, 20.0f, 0.1f } },
    { "Attack Radius", ScriptFieldType::Float, offsetof(EnemyCHASE, m_attackRadius), { 0.0f, 100.0f, 0.1f } },
    { "Lose Radius", ScriptFieldType::Float, offsetof(EnemyCHASE, m_loseRadius), { 0.0f, 200.0f, 0.1f } },
    { "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyCHASE, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyCHASE, CHASEFields)

EnemyCHASE::EnemyCHASE(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyCHASE::OnStateEnter()
{
    if (!m_debugEnabled)
    {
        return;
    }

    Debug::log("[EnemyCHASE] ENTER");
}

void EnemyCHASE::OnStateUpdate()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (!animation)
    {
        return;
    }

    GameObject* player = findPlayer();
    if (!player)
    {
        AnimationAPI::sendTrigger(animation, "Idle");

        if (m_debugEnabled)
        {
            Debug::log("[EnemyCHASE] Idle trigger sent (no player found)");
        }

        return;
    }

    Transform* playerTransform = GameObjectAPI::getTransform(player);
    if (!playerTransform)
    {
        return;
    }

    Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    Vector3 playerPosition = TransformAPI::getPosition(playerTransform);

    Vector3 toPlayer = playerPosition - ownerPosition;
    toPlayer.y = 0.0f;

    const float distanceSq = toPlayer.LengthSquared();
    const float attackRadiusSq = m_attackRadius * m_attackRadius;
    const float loseRadiusSq = m_loseRadius * m_loseRadius;

    if (distanceSq <= attackRadiusSq)
    {
        AnimationAPI::sendTrigger(animation, "Attack");

        if (m_debugEnabled)
        {
            Debug::log("[EnemyCHASE] Attack trigger sent");
        }

        return;
    }

    if (distanceSq >= loseRadiusSq)
    {
        AnimationAPI::sendTrigger(animation, "Idle");

        if (m_debugEnabled)
        {
            Debug::log("[EnemyCHASE] Idle trigger sent (player lost)");
        }

        return;
    }

    if (distanceSq > 0.0001f)
    {
        toPlayer.Normalize();
        const Vector3 delta = toPlayer * m_speed * Time::getDeltaTime();
        TransformAPI::translate(ownerTransform, delta);
    }
}

void EnemyCHASE::OnStateExit()
{
    if (!m_debugEnabled)
    {
        return;
    }

    Debug::log("[EnemyCHASE] EXIT");
}

GameObject* EnemyCHASE::findPlayer() const
{
    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    if (players.empty())
    {
        return nullptr;
    }

    return players.front();
}

IMPLEMENT_SCRIPT(EnemyCHASE)