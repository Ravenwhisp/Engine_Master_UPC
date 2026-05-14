#pragma once
#include "ImporterNative.h"
#include "SkinAsset.h"

class ImporterSkin : public ImporterNative<SkinAsset, AssetType::SKIN>
{
public:
    bool canImport(const std::filesystem::path&) const override
    {
        return false;
    }

    Asset* createAssetInstance(const MD5Hash& uid) const override
    {
        return new SkinAsset(uid);
    }

protected:
    bool importNative(const std::filesystem::path&, SkinAsset*) override
    {
        return false;
    }

    uint64_t saveTyped(const SkinAsset* source, uint8_t** outBuffer) override;
    void loadTyped(const uint8_t* buffer, SkinAsset* dst) override;
};
