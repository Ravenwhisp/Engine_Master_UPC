#pragma once

#include "IArchive.h"

#include <algorithm>

enum class VolumetricFogDebugView : uint32_t
{
    Final = 0,
    Medium = 1,
    Lighting = 2,
    LightingNoShadows = 3,
    AccumulatedScattering = 4,
    Transmittance = 5
};

struct VolumetricFogSettings
{
    VolumetricFogDebugView debugView = VolumetricFogDebugView::Final;
    float debugSlice = 0.5f;

    bool enabled = false;

    // Dimensionless global density scale.
    // Effective medium coefficients will be density * coefficient.
    float density = 1.0f;

    // Participating-medium coefficients per world-distance unit.
    float scatteringCoefficient = 0.01f;
    float extinctionCoefficient = 0.02f;

    // Henyey-Greenstein anisotropy factor.
    float anisotropy = 0.0f;

    // Maximum world-space distance covered by the volumetric volume.
    float maxDistance = 100.0f;

    bool animateDensity = false;
    float noiseScale = 0.05f;
    float noiseStrength = 0.5f;
    float windDirectionX = 1.0f;
    float windDirectionY = 0.0f;
    float windDirectionZ = 0.0f;
    float windSpeed = 0.25f;

    void sanitize()
    {
        density = (std::max)(0.0f, density);
        scatteringCoefficient = (std::max)(0.0f, scatteringCoefficient);
        extinctionCoefficient = (std::max)(scatteringCoefficient, extinctionCoefficient);
        anisotropy = std::clamp(anisotropy, -0.99f, 0.99f);
        maxDistance = (std::max)(0.1f, maxDistance);
        debugSlice = std::clamp(debugSlice, 0.0f, 1.0f);
        noiseScale = std::max(noiseScale, 0.0001f);
        noiseStrength = std::clamp(noiseStrength, 0.0f, 1.0f);
        windSpeed = std::max(windSpeed, 0.0f);
        if (static_cast<uint32_t>(debugView) > static_cast<uint32_t>(VolumetricFogDebugView::Transmittance)) debugView = VolumetricFogDebugView::Final;
    }

    void serialize(IArchive& archive)
    {
        archive.serialize(enabled, "Enabled");
        archive.serialize(density, "Density");
        archive.serialize(scatteringCoefficient, "ScatteringCoefficient");
        archive.serialize(extinctionCoefficient, "ExtinctionCoefficient");
        archive.serialize(anisotropy, "Anisotropy");
        archive.serialize(maxDistance, "MaxDistance");
        archive.serialize(animateDensity, "AnimateDensity");
        archive.serialize(noiseScale, "NoiseScale");
        archive.serialize(noiseStrength, "NoiseStrength");
        archive.serialize(windDirectionX, "WindDirectionX");
        archive.serialize(windDirectionY, "WindDirectionY");
        archive.serialize(windDirectionZ, "WindDirectionZ");
        archive.serialize(windSpeed, "WindSpeed");

        uint32_t debugViewValue = static_cast<uint32_t>(debugView);
        archive.serialize(debugViewValue, "DebugView");
        archive.serialize(debugSlice, "DebugSlice");

        if (archive.mode() == ArchiveMode::Input)
        {
            debugView = debugViewValue <= static_cast<uint32_t>(VolumetricFogDebugView::Transmittance) ? static_cast<VolumetricFogDebugView>(debugViewValue) : VolumetricFogDebugView::Final;
            sanitize();
        }

    }
};