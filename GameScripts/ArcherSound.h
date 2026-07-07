#pragma once

#include "EnemySound.h"

// Archer SFX. Lives on the Archer GameObject (needs a SOUND_SOURCE).
class ArcherSound : public EnemySound
{
    DECLARE_SCRIPT(ArcherSound)

public:
    explicit ArcherSound(GameObject* owner);

    // Arrow Barrage has no dedicated SFX, so we reuse the basic shot (a random
    // container) fired as a quick 4-shot volley: once on release, once on impact.
    void playBarrageReleaseVolley();
    void playBarrageImpactVolley();

protected:
    const char* evBasicTelegraph() const override;
    const char* evBasicImpact()    const override;
    const char* evHurt()           const override;
    const char* evStun()           const override;
    const char* evDeath()          const override;
    const char* evFootstep()       const override;
};
