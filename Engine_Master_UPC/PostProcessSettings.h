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
    }
};
