#include "pch.h"
#include "ArcherSound.h"

namespace
{
    constexpr const char* k_basicRelease = "Play_Archer_Basic_Release";
    constexpr const char* k_basicImpact  = "Play_Archer_Basic_Impact";
    constexpr const char* k_hurt         = "Play_Archer_Hurt";
    constexpr const char* k_stun         = "Play_Archer_Stun";
    constexpr const char* k_death        = "Play_Archer_Death";
    constexpr const char* k_footstep     = "Play_ArcherFootsteps";

    // Volley timing. The arrows reuse the basic shot (Wwise random container → a distinct
    // sample each time), so the only thing that made it sound mechanical was the perfectly
    // even spacing. These UNEVEN offsets (gaps that widen slightly) read as a natural
    // flurry of looses, then a spread-out rain of impacts — not a "ta-ta-ta-ta" stutter.
    constexpr float k_releaseOffsets[] = { 0.0f, 0.05f, 0.11f, 0.18f }; // quick loose (~180 ms)
    constexpr float k_impactOffsets[]  = { 0.0f, 0.09f, 0.20f, 0.34f }; // arrows raining (~340 ms)
}

ArcherSound::ArcherSound(GameObject* owner)
    : EnemySound(owner)
{
}

const char* ArcherSound::evBasicTelegraph() const { return k_basicRelease; }
const char* ArcherSound::evBasicImpact()    const { return k_basicImpact; }
const char* ArcherSound::evHurt()           const { return k_hurt; }
const char* ArcherSound::evStun()           const { return k_stun; }
const char* ArcherSound::evDeath()          const { return k_death; }
const char* ArcherSound::evFootstep()       const { return k_footstep; }

void ArcherSound::playBarrageReleaseVolley()
{
    for (const float offset : k_releaseOffsets)
    {
        postEventDelayed(k_basicRelease, offset);
    }
}

void ArcherSound::playBarrageImpactVolley()
{
    for (const float offset : k_impactOffsets)
    {
        postEventDelayed(k_basicImpact, offset);
    }
}

IMPLEMENT_SCRIPT(ArcherSound)
