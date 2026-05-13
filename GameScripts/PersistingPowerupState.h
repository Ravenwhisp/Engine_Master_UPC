#pragma once

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

private:
    static bool s_unlockedPowerups[static_cast<int>(PowerupId::Count)];
};