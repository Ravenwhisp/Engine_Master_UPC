#pragma once

#include "ScriptAPI.h"

class PlayerState;
class PlayerController;
class PlayerRotation;
class PlayerAnimationController;
class PlayerTargetController;
class Damageable;
class AbilityBase;

class CharacterBase : public Script
{
public:
    explicit CharacterBase(GameObject* owner);

    void Start() override;

    int getPlayerIndex() const;

    PlayerState* getPlayerState() const { return m_playerState; }
    PlayerController* getPlayerController() const { return m_playerController; }
    PlayerRotation* getPlayerRotation() const { return m_playerRotation; }
    PlayerAnimationController* getAnimationController() const { return m_playerAnimationController; }
    PlayerTargetController* getTargetController() const { return m_targetController; }
    Damageable* getDamageable() const { return m_damageable; }

    AbilityBase* getBasicAttack() const { return m_basicAttack; }
    AbilityBase* getChargedAttack() const { return m_chargedAttack; }
    AbilityBase* getSpecialAbility() const { return m_specialAbility; }

    bool isDowned() const;
    bool isUsingAbility() const;
    void setUsingAbility(bool value);

protected:
    PlayerState* m_playerState = nullptr;
    PlayerController* m_playerController = nullptr;
    PlayerRotation* m_playerRotation = nullptr;
    PlayerAnimationController* m_playerAnimationController = nullptr;
    PlayerTargetController* m_targetController = nullptr;
    Damageable* m_damageable = nullptr;

    AbilityBase* m_basicAttack = nullptr;
    AbilityBase* m_chargedAttack = nullptr;
    AbilityBase* m_specialAbility = nullptr;
};