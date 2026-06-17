#pragma once

#include "AbilityBase.h"

class DeathCharacter;
class DeathConfig;

class DeathAbilityBase : public AbilityBase
{
public:
    explicit DeathAbilityBase(GameObject* owner);

    void Start() override;

protected:
    void releaseComboMoveLock();

protected:
    DeathCharacter* m_deathCharacter = nullptr;
    DeathConfig* m_config = nullptr;

    bool m_movementLockedForCombo = false;
};
