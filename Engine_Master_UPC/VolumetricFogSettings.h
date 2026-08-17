#pragma once

#include "IArchive.h"

#include <algorithm>

struct VolumetricFogSettings
{
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

    void sanitize()
    {
        density = (std::max)(0.0f, density);
        scatteringCoefficient = (std::max)(0.0f, scatteringCoefficient);
        extinctionCoefficient = (std::max)(scatteringCoefficient, extinctionCoefficient);
        anisotropy = std::clamp(anisotropy, -0.99f, 0.99f);
        maxDistance = (std::max)(0.1f, maxDistance);
    }

    void serialize(IArchive& archive)
    {
        archive.serialize(enabled, "Enabled");
        archive.serialize(density, "Density");
        archive.serialize(scatteringCoefficient, "ScatteringCoefficient");
        archive.serialize(extinctionCoefficient, "ExtinctionCoefficient");
        archive.serialize(anisotropy, "Anisotropy");
        archive.serialize(maxDistance, "MaxDistance");

        if (archive.mode() == ArchiveMode::Input)
        {
            sanitize();
        }
    }
};