#pragma once
#include "ImporterSource.h"
#include "SoundBankAsset.h"
#include "Extensions.h"
#include <vector>
#include <string>
#include <cstdint>

struct SoundBankSourceData
{
    std::string bankName;
    std::vector<WwiseEvent> events;
    std::vector<uint8_t> bankData;
};

class ImporterSoundBank : public ImporterSource<SoundBankSourceData, SoundBankAsset, AssetType::SOUND_BANK>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension() == BNK_EXTENSION;
    }

    Asset* createAssetInstance(AssetId& uid) const override
    {
        return new SoundBankAsset(uid);
    }

protected:
    bool loadExternal(const std::filesystem::path& path, SoundBankSourceData& out) override;
    void importTyped(const SoundBankSourceData& source, SoundBankAsset* dst) override;
};
