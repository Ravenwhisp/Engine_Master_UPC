#include "pch.h"
#include "EnemyATTACK.h"

static const ScriptFieldInfo ATTACKFields[] =
{
    { "Attack Radius", ScriptFieldType::Float, offsetof(EnemyATTACK, m_attackRadius), { 0.0f, 100.0f, 0.1f } },
    { "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyATTACK, m_debugEnabled) }
};

IMPLEMENT_SCRIPT_FIELDS(EnemyATTACK, ATTACKFields)

EnemyATTACK::EnemyATTACK(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyATTACK::OnStateEnter()
{
    if (!m_debugEnabled)
    {
        return;
    }

    Debug::log("[EnemyATTACK] ENTER");
}

void EnemyATTACK::OnStateUpdate()
{
    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (!animation)
    {
        return;
    }

    GameObject* player = findPlayer();
    if (!player)
    {
        AnimationAPI::sendTrigger(animation, "Chase");

        if (m_debugEnabled)
        {
            Debug::log("[EnemyATTACK] Chase trigger sent (no player found)");
        }

        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    Transform* playerTransform = GameObjectAPI::getTransform(player);

    if (!ownerTransform || !playerTransform)
    {
        return;
    }

    Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    Vector3 playerPosition = TransformAPI::getPosition(playerTransform);

    Vector3 toPlayer = playerPosition - ownerPosition;
    toPlayer.y = 0.0f;

    const float distanceSq = toPlayer.LengthSquared();
    const float attackRadiusSq = m_attackRadius * m_attackRadius;

    if (distanceSq > attackRadiusSq)
    {
        AnimationAPI::sendTrigger(animation, "Chase");

        if (m_debugEnabled)
        {
            Debug::log("[EnemyATTACK] Chase trigger sent (player out of attack range)");
        }
    }
}

void EnemyATTACK::OnStateExit()
{
    if (!m_debugEnabled)
    {
        return;
    }

    Debug::log("[EnemyATTACK] EXIT");
}

GameObject* EnemyATTACK::findPlayer() const
{
    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    if (players.empty())
    {
        return nullptr;
    }

    return players.front();
}

IMPLEMENT_SCRIPT(EnemyATTACK)