#include "pch.h"
#include "PlayerTargetController.h"

#include "CharacterBase.h"

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
    Script* characterScript = GameObjectAPI::getScript(getOwner(), "LyrielCharacter");
    if (characterScript == nullptr)
    {
        characterScript = GameObjectAPI::getScript(getOwner(), "DeathCharacter");
    }

    m_character = static_cast<CharacterBase*>(characterScript);

    if (m_character == nullptr)
    {
        Debug::warn("PlayerTargetController on '%s' could not find CharacterBase-derived script on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }
}

void PlayerTargetController::Update()
{
    updateEnemiesInRange();
    ensureValidCurrentTarget();

    if (m_character == nullptr)
    {
        return;
    }

    if (Input::isRightStickJustPressed(m_character->getPlayerIndex()))
    {
        cycleTarget();
    }
}

void PlayerTargetController::drawGizmo()
{
    using namespace DebugDrawAPI;

    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    if (ownerTransform == nullptr)
    {
        return;
    }

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

    if (ownerTransform == nullptr || enemyTransform == nullptr)
    {
        return false;
    }

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