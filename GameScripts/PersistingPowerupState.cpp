#include "pch.h"
#include "PersistingPowerupState.h"

bool PersistingPowerupState::s_unlockedPowerups[static_cast<int>(PowerupId::Count)] = {};

void PersistingPowerupState::unlock(PowerupId powerup)
{
    const int index = static_cast<int>(powerup);

    if (index < 0 || index >= static_cast<int>(PowerupId::Count))
    {
        return;
    }

    s_unlockedPowerups[index] = true;
}

bool PersistingPowerupState::isUnlocked(PowerupId powerup)
{
    const int index = static_cast<int>(powerup);

    if (index < 0 || index >= static_cast<int>(PowerupId::Count))
    {
        return false;
    }

    return s_unlockedPowerups[index];
}

void PersistingPowerupState::reset()
{
    for (int i = 0; i < static_cast<int>(PowerupId::Count); ++i)
    {
        s_unlockedPowerups[i] = false;
    }
}