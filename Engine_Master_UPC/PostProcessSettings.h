#pragma once
#include "ISerializable.h"
#include "IArchive.h"
#include <string>

struct PostProcessSettings : public ISerializable
{
    float exposure = 0.0f;

    // Bloom
    bool  bloomEnabled = false;
    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;

    // Colour grading 
    bool        lutEnabled = false;
    std::string lutPath;

    // Chromatic aberration
    bool  chromaticAberrationEnabled = false;
    float chromaticAberrationStrength = 1.0f;

    // Heartbeat / low-health "damage screen" effect.
    bool  heartbeatEnabled = false;
    float healthThreshold = 0.5f;   // health below this triggers the effect
    // Runtime inputs (normally driven by gameplay: PlayerDamageable / Bound).
    float health = 1.0f;            // 0..1
    float separation = 0.0f;        // 0..1

    // Death fade: desaturate to grey, then fade fully to black. Triggered by
    // gameplay when every player is down (health 0, death animation playing).
    bool  deathFadeActive = false;  // runtime trigger
    float deathGreyDuration = 1.5f; // seconds to reach full grey
    float deathBlackDuration = 1.5f;// seconds to fade from grey to black

    void serialize(IArchive& archive) override
    {
        archive.serialize(exposure, "exposure");

        archive.serialize(bloomEnabled, "bloomEnabled");
        archive.serialize(bloomThreshold, "bloomThreshold");
        archive.serialize(bloomIntensity, "bloomIntensity");

        archive.serialize(lutEnabled, "lutEnabled");
        archive.serialize(lutPath, "lutPath");

        archive.serialize(chromaticAberrationEnabled, "chromaticAberrationEnabled");
        archive.serialize(chromaticAberrationStrength, "chromaticAberrationStrength");

        // Only the authored knobs persist; health/separation are runtime inputs.
        archive.serialize(heartbeatEnabled, "heartbeatEnabled");
        archive.serialize(healthThreshold, "healthThreshold");

        archive.serialize(deathGreyDuration, "deathGreyDuration");
        archive.serialize(deathBlackDuration, "deathBlackDuration");
    }
};
