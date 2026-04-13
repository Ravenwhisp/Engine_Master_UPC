#include "pch.h"
#include "EnemyIDLE.h"

static const ScriptFieldInfo IDLEFields[] =
{
    { "Speed", ScriptFieldType::Float, offsetof(EnemyIDLE, m_speed), { 0.0f, 20.0f, 0.1f } },
    { "Debug Enabled", ScriptFieldType::Bool, offsetof(EnemyIDLE, m_debugEnabled) },
    { "Detection Radius", ScriptFieldType::Float, offsetof(EnemyIDLE, m_detectionRadius), { 0.0f, 100.0f, 0.1f } },
};

IMPLEMENT_SCRIPT_FIELDS(EnemyIDLE, IDLEFields)


EnemyIDLE::EnemyIDLE(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyIDLE::OnStateEnter()
{
    if (!m_debugEnabled)
    {
        return;
    }

    Debug::log("[EnemyIDLE] ENTER");
}

void EnemyIDLE::OnStateUpdate()
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    GameObject* player = findPlayer();
    if (!player)
    {
        return;
    }

    Transform* playerTransform = GameObjectAPI::getTransform(player);
    if (!playerTransform)
    {
        return;
    }

    const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    const Vector3 playerPosition = TransformAPI::getPosition(playerTransform);

    const Vector3 toPlayer = playerPosition - ownerPosition;
    const float distanceSq = toPlayer.LengthSquared();
    const float detectionRadiusSq = m_detectionRadius * m_detectionRadius;

    if (distanceSq <= detectionRadiusSq)
    {
        AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
        if (!animation)
        {
            return;
        }

        AnimationAPI::sendTrigger(animation, "Chase");

        if (m_debugEnabled)
        {
            Debug::log("[EnemyIDLE] Chase trigger sent");
        }
    }
}

void EnemyIDLE::OnStateExit()
{
    if (!m_debugEnabled)
    {
        return;
    }

    Debug::log("[EnemyIDLE] EXIT");
}

GameObject* EnemyIDLE::findPlayer() const
{
    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER);

    if (players.empty())
    {
        return nullptr;
    }

    return players.front();
}

IMPLEMENT_SCRIPT(EnemyIDLE)