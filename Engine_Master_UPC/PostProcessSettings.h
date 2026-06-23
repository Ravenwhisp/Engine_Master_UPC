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
    float healthThreshold = 0.5f;   
    float health = 1.0f;     
    float separation = 0.0f; 

    // Death fade: desaturate to grey, then fade fully to black. 
    bool  deathFadeActive = false; 
    float deathGreyDuration = 1.5f; 
    float deathBlackDuration = 1.5f;

    bool  outlineEnabled = false;
    float outlineThickness = 1.5f; 
    float outlineThreshold = 0.02f;
    float outlineIntensity = 0.6f;  
    float outlineColorR = 0.05f;  
    float outlineColorG = 0.04f;
    float outlineColorB = 0.05f;
    float outlineWobble = 1.0f;    
    float outlineNoiseScale = 90.0f;
    float outlineBreakup = 0.5f;   

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

        archive.serialize(heartbeatEnabled, "heartbeatEnabled");
        archive.serialize(healthThreshold, "healthThreshold");

        archive.serialize(deathGreyDuration, "deathGreyDuration");
        archive.serialize(deathBlackDuration, "deathBlackDuration");

        archive.serialize(outlineEnabled, "outlineEnabled");
        archive.serialize(outlineThickness, "outlineThickness");
        archive.serialize(outlineThreshold, "outlineThreshold");
        archive.serialize(outlineIntensity, "outlineIntensity");
        archive.serialize(outlineColorR, "outlineColorR");
        archive.serialize(outlineColorG, "outlineColorG");
        archive.serialize(outlineColorB, "outlineColorB");
        archive.serialize(outlineWobble, "outlineWobble");
        archive.serialize(outlineNoiseScale, "outlineNoiseScale");
        archive.serialize(outlineBreakup, "outlineBreakup");
    }
};
