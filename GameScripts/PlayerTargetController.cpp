#include "pch.h"
#include "PlayerTargetController.h"

#include "CharacterBase.h"
#include "Damageable.h"
#include "EnemyDamageable.h"
#include "BreakableDamageable.h"

IMPLEMENT_SCRIPT_FIELDS(PlayerTargetController,
    SERIALIZED_FLOAT(m_targetRange, "Target Range", 0.0f, 20.0f, 0.05f)
)

PlayerTargetController::PlayerTargetController(GameObject* owner)
    : Script(owner)
{
}

void PlayerTargetController::Start()
{
    m_character = GameObjectAPI::findScript<CharacterBase>(getOwner());

    if (m_character == nullptr)
    {
        Debug::warn("PlayerTargetController on '%s' could not find CharacterBase-derived script on the same GameObject.", GameObjectAPI::getName(getOwner()));
    }
}

void PlayerTargetController::Update()
{
    updateTargetsInRange();
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

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);

    const Vector3 green = { 0.0f, 1.0f, 0.0f };
    const Vector3 yellow = { 1.0f, 1.0f, 0.0f };

    drawCircle(ownerPosition, Vector3(0.0f, 1.0f, 0.0f), green, m_targetRange, 32.0f, 0, true);

    if (m_currentTarget != nullptr)
    {
        Transform* targetTransform = GameObjectAPI::getTransform(m_currentTarget);
        if (targetTransform != nullptr)
        {
            const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);
            drawLine(ownerPosition, targetPosition, yellow, 0, true);
        }
    }
}

void PlayerTargetController::updateTargetsInRange()
{
    m_targetsInRange.clear();

    const std::vector<GameObject*> enemies = SceneAPI::findAllGameObjectsByTag(Tag::ENEMY, true);
    const std::vector<GameObject*> breakables = SceneAPI::findAllGameObjectsByTag(Tag::BREAKABLE, true);

    for (GameObject* enemy : enemies)
    {
        if (enemy == nullptr)
        {
            continue;
        }

        const bool inRange = isTargetInRange(enemy);

        if (inRange && isTargetAlive(enemy))
        {
            m_targetsInRange.push_back(enemy);
        }
    }

    for (GameObject* breakable : breakables)
    {
        if (breakable == nullptr)
        {
            continue;
        }

        const bool inRange = isTargetInRange(breakable);

        if (inRange && isTargetAlive(breakable))
        {
            m_targetsInRange.push_back(breakable);
        }
    }
}

void PlayerTargetController::ensureValidCurrentTarget()
{
    GameObject* previousTarget = m_currentTarget;

    if (m_targetsInRange.empty())
    {
        m_currentTarget = nullptr;
    }
    else if (findTargetIndex(m_currentTarget) == -1)
    {
        m_currentTarget = m_targetsInRange[0];
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
    if (m_targetsInRange.empty())
    {
        m_currentTarget = nullptr;
        Debug::log("No targets in range");
        return;
    }

    const int currentIndex = findTargetIndex(m_currentTarget);

    if (currentIndex == -1)
    {
        m_currentTarget = m_targetsInRange[0];
    }
    else
    {
        const int nextIndex = (currentIndex + 1) % static_cast<int>(m_targetsInRange.size());
        m_currentTarget = m_targetsInRange[nextIndex];
    }

    Debug::log("Cycled target: %s", GameObjectAPI::getName(m_currentTarget));
}

bool PlayerTargetController::isTargetInRange(GameObject* target) const
{
    if (target == nullptr)
    {
        return false;
    }

    GameObject* owner = getOwner();

    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    Transform* targetTransform = GameObjectAPI::getTransform(target);

    if (ownerTransform == nullptr || targetTransform == nullptr)
    {
        return false;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    const Vector3 distanceFromTarget = targetPosition - ownerPosition;
    const float distance = distanceFromTarget.Length();

    return distance <= m_targetRange;
}

bool PlayerTargetController::isTargetAlive(GameObject* target) const
{
    if (target == nullptr)
    {
        return false;
    }

    Script* enemyDamageableScript = GameObjectAPI::getScript(target, "EnemyDamageable");
    EnemyDamageable* enemyDamageable = dynamic_cast<EnemyDamageable*>(enemyDamageableScript);

    if (enemyDamageable != nullptr)
    {
        return !enemyDamageable->isDead() && enemyDamageable->getCurrentHp() > 0.0f;
    }

    Script* breakableDamageableScript = GameObjectAPI::getScript(target, "BreakableDamageable");
    BreakableDamageable* breakableDamageable = dynamic_cast<BreakableDamageable*>(breakableDamageableScript);

    if (breakableDamageable != nullptr)
    {
        return !breakableDamageable->isDead() && breakableDamageable->getCurrentHp() > 0.0f;
    }

    Script* damageableScript = GameObjectAPI::getScript(target, "Damageable");
    Damageable* damageable = dynamic_cast<Damageable*>(damageableScript);

    if (damageable != nullptr)
    {
        return !damageable->isDead() && damageable->getCurrentHp() > 0.0f;
    }

    return false;
}

int PlayerTargetController::findTargetIndex(GameObject* target) const
{
    if (target == nullptr)
    {
        return -1;
    }

    for (int i = 0; i < static_cast<int>(m_targetsInRange.size()); ++i)
    {
        if (m_targetsInRange[i] == target)
        {
            return i;
        }
    }

    return -1;
}

IMPLEMENT_SCRIPT(PlayerTargetController)