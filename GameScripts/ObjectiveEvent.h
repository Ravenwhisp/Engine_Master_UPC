#pragma once

#include "ScriptAPI.h"
#include "GameplayEventAction.h"

#include "ObjectiveTypes.h"

class GameplayEventTrigger;
class PopUpController;
class CharacterBase;
class PlayerMovement;
class AbilityBase;

class ObjectiveEvent : public GameplayEventAction
{
    DECLARE_SCRIPT(ObjectiveEvent)

public:
    explicit ObjectiveEvent(GameObject* owner);

    void Start() override;
    void Update() override;

    void executeEvent(GameplayEventTrigger* trigger) override;

    ScriptFieldList getExposedFields() const override;

private:
    bool isObjectiveCompleted() const;

    bool isMovementCompleted() const;
    bool isAutoAttackCompleted() const;
    bool isChargedAttackCompleted() const;
    bool isAbilityCompleted() const;
    bool isDashCompleted() const;
    bool isBreakableObjectsCompleted() const;

    int countBrokenBreakables() const;

    void findTargetPlayer();
    PopUpController* findPopUpController() const;

public:
    int m_objectiveType = static_cast<int>(ObjectiveType::Movement);
    int m_targetPlayerIndex = 0;
    int m_targetBreakableCount = 1;

private:
    CharacterBase* m_targetCharacter = nullptr;
    PlayerMovement* m_targetPlayerMovement = nullptr;
    AbilityBase* m_targetBasicAttack = nullptr;
    AbilityBase* m_targetChargedAttack = nullptr;
    AbilityBase* m_targetSpecialAbility = nullptr;

    bool m_isActive = false;
    bool m_hasCompleted = false;

    int m_initialBasicAttackUseCount = 0;
    int m_initialChargedAttackUseCount = 0;
    int m_initialSpecialAbilityUseCount = 0;

    AbilityBase* m_targetDash = nullptr;
    int m_initialDashUseCount = 0;

    int m_initialBrokenCount = 0;
};