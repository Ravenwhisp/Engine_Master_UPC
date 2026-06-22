#pragma once

#include <cstdint>

#include "IArchive.h"
#include "ISerializable.h"

static constexpr uint32_t SSAO_KERNEL_SIZE = 32;

struct SSAOSettings : public ISerializable
{
    bool enabled = true;
    bool blurEnabled = true;
    bool debugView = false;

    float radius = 0.5f;
    float bias = 0.025f;
    float strength = 1.0f;

    uint32_t sampleCount = SSAO_KERNEL_SIZE;

    void serialize(IArchive& archive) override
    {
        archive.serialize(enabled, "enabled");
        archive.serialize(blurEnabled, "blurEnabled");
        archive.serialize(debugView, "debugView");

        archive.serialize(radius, "radius");
        archive.serialize(bias, "bias");
        archive.serialize(strength, "strength");

        archive.serialize(sampleCount, "sampleCount");
    }
};