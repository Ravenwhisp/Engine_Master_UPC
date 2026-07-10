#pragma once

#include "EnemySound.h"

// Paladin SFX. Lives on the Paladin GameObject (needs a SOUND_SOURCE).
class PaladinSound : public EnemySound
{
    DECLARE_SCRIPT(PaladinSound)

public:
    explicit PaladinSound(GameObject* owner);

    // Charge ability (called by PaladinChargeState).
    void playChargeStart();
    void startChargeLoop();
    void stopChargeLoop();
    void playChargeImpact();   // only when the charge connects

    void stopAllLoops() override;

protected:
    const char* evBasicTelegraph() const override;
    const char* evBasicImpact()    const override;
    const char* evHurt()           const override;
    const char* evStun()           const override;
    const char* evDeath()          const override;
    const char* evFootstep()       const override;

private:
    uint32_t m_chargeLoopID = 0;
};
