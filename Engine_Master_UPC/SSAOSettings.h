#pragma once

#include <algorithm>
#include <cstdint>

#include "IArchive.h"
#include "ISerializable.h"

static constexpr uint32_t SSAO_KERNEL_SIZE = 32;

struct SSAOSettings : public ISerializable
{
    bool enabled = true;
    bool blurEnabled = true;
    bool debugView = false;

    float radius = 1.0f;
    float bias = 0.001f;
    float strength = 2.0f;

    uint32_t sampleCount = SSAO_KERNEL_SIZE;

    void clampValues()
    {
        radius = std::clamp(radius, 0.01f, 5.0f);
        bias = std::clamp(bias, 0.0f, 0.1f);
        strength = std::clamp(strength, 0.0f, 8.0f);
        sampleCount = std::clamp<uint32_t>(sampleCount, 1u, SSAO_KERNEL_SIZE);
    }

    void serialize(IArchive& archive) override
    {
        clampValues();

        archive.serialize(enabled, "enabled");
        archive.serialize(blurEnabled, "blurEnabled");
        archive.serialize(debugView, "debugView");

        archive.serialize(radius, "radius");
        archive.serialize(bias, "bias");
        archive.serialize(strength, "strength");

        archive.serialize(sampleCount, "sampleCount");

        clampValues();
    }
};