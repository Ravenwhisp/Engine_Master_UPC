#pragma once
#include "ISerializable.h"
#include "IArchive.h"
#include <string>
#include <vector>

struct SoundBanksData : public ISerializable
{
    std::vector<std::string> banks;

    void serialize(IArchive& archive) override
    {
        uint32_t count = static_cast<uint32_t>(banks.size());
        archive.beginArray(count, "banks");

        if (archive.mode() == ArchiveMode::Input)
            banks.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            archive.serialize(banks[i]);
        }

        archive.endArray();
    }
};
