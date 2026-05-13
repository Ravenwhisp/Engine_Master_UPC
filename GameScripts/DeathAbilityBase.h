#pragma once

#include "AbilityBase.h"

class DeathCharacter;

class DeathAbilityBase : public AbilityBase
{
public:
    explicit DeathAbilityBase(GameObject* owner);

    void Start() override;

protected:
    void releaseComboMoveLock();

protected:
    DeathCharacter* m_deathCharacter = nullptr;
    bool m_movementLockedForCombo = false;
};
