#include "pch.h"
#include "PlayerTargetController.h"

static const ScriptFieldInfo playerTargetControllerFields[] =
{
    { "Target Range", ScriptFieldType::Float, offsetof(PlayerTargetController, m_targetRange), { 0.0f, 20.0f, 0.05f } },
    { "Player Index", ScriptFieldType::Int, offsetof(PlayerTargetController, m_playerIndex) }
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
    updateEnemiesInRange();
    ensureValidCurrentTarget();

    if (Input::isRightStickJustPressed(m_playerIndex))
    {
        cycleTarget();
    }
}

void PlayerTargetController::drawGizmo()
{
    using namespace DebugDrawAPI;

    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);

    const Vector3 green = { 0.0f, 1.0f, 0.0f };
    const Vector3 yellow = { 1.0f, 1.0f, 0.0f };

    drawCircle(ownerPosition, Vector3(0.0f, 1.0f, 0.0f), green, m_targetRange, 32.0f, 0, true);

    if (m_currentTarget != nullptr)
    {
        Transform* targetTransform = GameObjectAPI::getTransform(m_currentTarget);
        if (targetTransform != nullptr)
        {
            const Vector3 targetPosition = TransformAPI::getPosition(targetTransform);
            drawLine(ownerPosition, targetPosition, yellow, 0, true);
        }
    }
}

void PlayerTargetController::updateEnemiesInRange()
{
    m_enemiesInRange.clear();

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);

    for (GameObject* enemy : enemies)
    {
        if (enemy == nullptr)
        {
            continue;
        }

        if (isEnemyInRange(enemy))
        {
            m_enemiesInRange.push_back(enemy);
        }
    }
}

void PlayerTargetController::ensureValidCurrentTarget()
{
    GameObject* previousTarget = m_currentTarget;

    if (m_enemiesInRange.empty())
    {
        m_currentTarget = nullptr;
    }
    else if (findTargetIndex(m_currentTarget) == -1)
    {
        m_currentTarget = m_enemiesInRange[0];
    }

    if (m_currentTarget != previousTarget)
    {
        if (m_currentTarget != nullptr)
        {
            Debug::log("Current target: %s", GameObjectAPI::getName(m_currentTarget));
        }
        else
        {
            Debug::log("No current target");
        }
    }
}

void PlayerTargetController::cycleTarget()
{
    if (m_enemiesInRange.empty())
    {
        m_currentTarget = nullptr;
        Debug::log("No enemies in range");
        return;
    }

    const int currentIndex = findTargetIndex(m_currentTarget);

    if (currentIndex == -1)
    {
        m_currentTarget = m_enemiesInRange[0];
    }
    else
    {
        const int nextIndex = (currentIndex + 1) % static_cast<int>(m_enemiesInRange.size());
        m_currentTarget = m_enemiesInRange[nextIndex];
    }

    Debug::log("Cycled target: %s", GameObjectAPI::getName(m_currentTarget));
}

bool PlayerTargetController::isEnemyInRange(GameObject* enemy) const
{
    if (enemy == nullptr)
    {
        return false;
    }

    GameObject* owner = getOwner();

    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    Transform* enemyTransform = GameObjectAPI::getTransform(enemy);

    const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
    const Vector3 enemyPosition = TransformAPI::getPosition(enemyTransform);

    const Vector3 distanceFromEnemy = enemyPosition - ownerPosition;
    const float distance = distanceFromEnemy.Length();

    return distance <= m_targetRange;
}

int PlayerTargetController::findTargetIndex(GameObject* target) const
{
    if (target == nullptr)
    {
        return -1;
    }

    for (int i = 0; i < static_cast<int>(m_enemiesInRange.size()); ++i)
    {
        if (m_enemiesInRange[i] == target)
        {
            return i;
        }
    }

    return -1;
}

IMPLEMENT_SCRIPT(PlayerTargetController)