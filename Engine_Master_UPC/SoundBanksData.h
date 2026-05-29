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
        archive.serialize(count, "count");

        if (archive.mode() == ArchiveMode::Input)
            banks.resize(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            std::string key = std::to_string(i);
            archive.serialize(banks[i], key.c_str());
        }
    }
};
