#pragma once

#include <cstdint>

struct ShadowCasterHandle
{
    static constexpr uint32_t INVALID_INDEX = 0xffffffffu;

    uint64_t registryId = 0;
    uint32_t index = INVALID_INDEX;
    uint32_t generation = 0;

    bool isFormed() const { return registryId != 0 && index != INVALID_INDEX; }

    void reset()
    {
        registryId = 0;
        index = INVALID_INDEX;
        generation = 0;
    }

    bool operator==(const ShadowCasterHandle& other) const
    {
        return registryId == other.registryId && index == other.index && generation == other.generation;
    }
};