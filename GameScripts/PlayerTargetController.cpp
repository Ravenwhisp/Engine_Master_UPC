#include "pch.h"
#include "PlayerTargetController.h"

static const ScriptFieldInfo playerTargetControllerFields[] =
{
    { "Target Range", ScriptFieldType::Float, offsetof(PlayerTargetController, m_targetRange), { 0.0f, 20.0f, 0.05f } }
};

IMPLEMENT_SCRIPT_FIELDS(PlayerTargetController, playerTargetControllerFields)

PlayerTargetController::PlayerTargetController(GameObject* owner)
    : Script(owner)
{
}

void PlayerTargetController::Start()
{
}

void PlayerTargetController::Update()
{
    updateAutoTarget();
}

void PlayerTargetController::updateAutoTarget()
{
    GameObject* previousTarget = m_currentTarget;
    m_currentTarget = findNearestEnemyInRange();

    if (m_currentTarget != previousTarget)
    {
        if (m_currentTarget != nullptr)
        {
            Debug::log("New target: %s", GameObjectAPI::getName(m_currentTarget));
        }
        else
        {
            Debug::log("Target cleared");
        }
    }
}

GameObject* PlayerTargetController::findNearestEnemyInRange() const
{
    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);

    GameObject* nearestEnemy = nullptr;
    float nearestDistance = m_targetRange;

    for (GameObject* enemy : enemies)
    {
        if (enemy == nullptr)
        {
            continue;
        }

        Transform* enemyTransform = GameObjectAPI::getTransform(enemy);

        const Vector3 enemyPosition = TransformAPI::getPosition(enemyTransform);
        const Vector3 distanceFromEnemy = enemyPosition - ownerPosition;
        const float distance = distanceFromEnemy.Length();

        if (distance > m_targetRange)
        {
            continue;
        }

        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestEnemy = enemy;
        }
    }

    return nearestEnemy;
}

IMPLEMENT_SCRIPT(PlayerTargetController)