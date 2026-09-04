#pragma once
#include <algorithm>

enum class PowerupId
{
    LyrielPowerup1 = 0,
    DeathPowerup1,
    Count
};

class PersistingPowerupState
{
public:
    static void unlock(PowerupId powerup);
    static bool isUnlocked(PowerupId powerup);
    static void reset();
	static bool* getUnlockedPowerupState() { return s_unlockedPowerups; }
    static void setUnlockedPowerupState(const bool* unlockedPowerups) 
    {
        constexpr size_t count = static_cast<size_t>(PowerupId::Count);
        std::copy(unlockedPowerups, unlockedPowerups + count, s_unlockedPowerups);
    }

private:
    static bool s_unlockedPowerups[static_cast<int>(PowerupId::Count)];
};