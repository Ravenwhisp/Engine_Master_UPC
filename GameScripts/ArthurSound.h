#pragma once

#include "EnemySound.h"

// Arthur (boss) SFX. Reuses the EnemySound base for footsteps (driven by the controller's
// moveTowardsTarget), hurt (driven by EnemyDamageable) and the delayed-event machinery.
// Arthur has no stun and his death roar is fired from the controller (not a generic death
// state), so evStun/evDeath/evBasic* return nullptr. Everything else is bespoke.
class ArthurSound : public EnemySound
{
    DECLARE_SCRIPT(ArthurSound)

public:
    explicit ArthurSound(GameObject* owner);

    // Roars (from ArthurBossController).
    void playIntroRoar();
    void playPhase2Roar();
    void playDeathRoar();

    // Heavy Swipe (claw combo).
    void playClawSwipe();
    void playClawImpact();

    // Side Sweep.
    void playSideSweep();
    void playSideImpact();

    // Charging Slam.
    void playPreparingGrowl();
    void playChargeSlam();
    void playBodyImpact();
    void startGallopingLoop();
    void stopGallopingLoop();

    // Earth Hammer.
    void playHammerPreparing();
    void playHammerImpact();

    void stopAllLoops() override;

protected:
    const char* evBasicTelegraph() const override { return nullptr; }
    const char* evBasicImpact()    const override { return nullptr; }
    const char* evHurt()           const override;
    const char* evStun()           const override { return nullptr; }
    const char* evDeath()          const override { return nullptr; }   // controller fires the death roar
    const char* evFootstep()       const override;

private:
    uint32_t m_gallopingLoopID = 0;
};
