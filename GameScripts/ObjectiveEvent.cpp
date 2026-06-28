#include "pch.h"
#include "ObjectiveEvent.h"

#include "GameplayEventTrigger.h"
#include "PopUpController.h"

#include "CharacterBase.h"
#include "PlayerMovement.h"
#include "AbilityBase.h"
#include "LyrielDash.h"
#include "DeathDash.h"
#include "BreakableObject.h"

static const char* objectiveTypeNames[] =
{
    "None",
    "Movement",
    "Auto Attack",
    "Charged Attack",
    "Ability",
    "Dash",
    "Breakables"
};

constexpr int objectiveTypeCount = 7;

IMPLEMENT_SCRIPT_FIELDS(ObjectiveEvent,
    SERIALIZED_ENUM_INT(m_objectiveType, "Objective Type", objectiveTypeNames, objectiveTypeCount),
    SERIALIZED_INT(m_targetPlayerIndex, "Target Player Index"),
    SERIALIZED_INT(m_targetBreakableCount, "Target Breakables Count")
)

ObjectiveEvent::ObjectiveEvent(GameObject* owner)
    : GameplayEventAction(owner)
{
}

void ObjectiveEvent::executeEvent(GameplayEventTrigger* trigger)
{
    findTargetPlayer();

    m_initialBasicAttackUseCount = m_targetBasicAttack != nullptr ? m_targetBasicAttack->getSuccessfulUse() : 0;

    m_initialChargedAttackUseCount = m_targetChargedAttack != nullptr ? m_targetChargedAttack->getSuccessfulUse() : 0;

    m_initialSpecialAbilityUseCount = m_targetSpecialAbility != nullptr ? m_targetSpecialAbility->getSuccessfulUse() : 0;

    m_initialDashUseCount = m_targetDash != nullptr ? m_targetDash->getSuccessfulUse() : 0;

    m_initialBrokenCount = countBrokenBreakables();

    m_isActive = true;
    m_hasCompleted = false;
}

void ObjectiveEvent::Start()
{
    findTargetPlayer();
}

void ObjectiveEvent::Update()
{
    if (!m_isActive || m_hasCompleted)
    {
        return;
    }

    if (!isObjectiveCompleted())
    {
        return;
    }

    PopUpController* popUpController = findPopUpController();

    if (popUpController == nullptr)
    {
        Debug::warn("ObjectiveEvent on '%s' could not find PopUpController.", GameObjectAPI::getName(getOwner()));
        return;
    }

    popUpController->notifyObjectiveCompleted(getOwner());

    m_hasCompleted = true;
    m_isActive = false;
}

bool ObjectiveEvent::isObjectiveCompleted() const
{
    const ObjectiveType objectiveType = static_cast<ObjectiveType>(m_objectiveType);

    switch (objectiveType)
    {
    case ObjectiveType::Movement:
        return isMovementCompleted();

    case ObjectiveType::AutoAttack:
        return isAutoAttackCompleted();

    case ObjectiveType::ChargedAttack:
        return isChargedAttackCompleted();

    case ObjectiveType::Ability:
        return isAbilityCompleted();

    case ObjectiveType::Dash:
        return isDashCompleted();

    case ObjectiveType::BreakableObjects:
        return isBreakableObjectsCompleted();

    case ObjectiveType::None:
    default:
        return false;
    }
}

bool ObjectiveEvent::isMovementCompleted() const
{
    if (m_targetPlayerMovement == nullptr)
    {
        return false;
    }

    return m_targetPlayerMovement->isMoving();
}

bool ObjectiveEvent::isAutoAttackCompleted() const
{
    if (m_targetBasicAttack == nullptr)
    {
        return false;
    }

    return m_targetBasicAttack->getSuccessfulUse() > m_initialBasicAttackUseCount;
}

bool ObjectiveEvent::isChargedAttackCompleted() const
{
    if (m_targetChargedAttack == nullptr)
    {
        return false;
    }

    return m_targetChargedAttack->getSuccessfulUse() > m_initialChargedAttackUseCount;
}

bool ObjectiveEvent::isAbilityCompleted() const
{
    if (m_targetSpecialAbility == nullptr)
    {
        return false;
    }

    return m_targetSpecialAbility->getSuccessfulUse() > m_initialSpecialAbilityUseCount;
}

bool ObjectiveEvent::isDashCompleted() const
{
    if (m_targetDash == nullptr)
    {
        return false;
    }

    return m_targetDash->getSuccessfulUse() > m_initialDashUseCount;
}

bool ObjectiveEvent::isBreakableObjectsCompleted() const
{
    return countBrokenBreakables() >= m_initialBrokenCount + m_targetBreakableCount;
}

int ObjectiveEvent::countBrokenBreakables() const
{
    int count = 0;

    const std::vector<GameObject*> breakableObjects = SceneAPI::findAllGameObjectsByTag(Tag::BREAKABLE, true);

    for (GameObject* obj : breakableObjects)
    {
        if (obj == nullptr)
        {
            continue;
        }

        BreakableObject* breakable = GameObjectAPI::findScript<BreakableObject>(obj);

        if (breakable == nullptr)
        {
            continue;
        }

        if (breakable->isBroken())
        {
            ++count;
        }
    }

    return count;
}

void ObjectiveEvent::findTargetPlayer()
{
    m_targetCharacter = nullptr;
    m_targetPlayerMovement = nullptr;
    m_targetBasicAttack = nullptr;
    m_targetChargedAttack = nullptr;
    m_targetSpecialAbility = nullptr;

    const std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER, true);

    for (GameObject* player : players)
    {
        if (player == nullptr)
        {
            continue;
        }

        CharacterBase* character = GameObjectAPI::findScript<CharacterBase>(player);

        if (character == nullptr)
        {
            continue;
        }

        if (character->getPlayerIndex() != m_targetPlayerIndex)
        {
            continue;
        }

        m_targetCharacter = character;
        m_targetPlayerMovement = GameObjectAPI::findScript<PlayerMovement>(player);
        m_targetBasicAttack = character->getBasicAttack();
        m_targetChargedAttack = character->getChargedAttack();
        m_targetSpecialAbility = character->getSpecialAbility();

        m_targetDash = GameObjectAPI::findScript<LyrielDash>(player);
        if (m_targetDash == nullptr)
        {
            m_targetDash = GameObjectAPI::findScript<DeathDash>(player);
        }

        return;
    }

    Debug::warn("ObjectiveEvent on '%s' could not find player with index %d.", GameObjectAPI::getName(getOwner()), m_targetPlayerIndex);
}

PopUpController* ObjectiveEvent::findPopUpController() const
{
    const std::vector<GameObject*> popUpControllerObjects = SceneAPI::findAllGameObjectsWithScript<PopUpController>();

    if (popUpControllerObjects.empty())
    {
        return nullptr;
    }

    return GameObjectAPI::findScript<PopUpController>(popUpControllerObjects[0]);
}

IMPLEMENT_SCRIPT(ObjectiveEvent)