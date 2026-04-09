#pragma once
#include "ImporterNative.h"
#include "AnimationAsset.h"

class ImporterAnimation : public ImporterNative<AnimationAsset, AssetType::ANIMATION>
{
public:
    bool canImport(const std::filesystem::path&) const override
    {
        return false;
    }

    Asset* createAssetInstance(const MD5Hash& uid) const override
    {
        return new AnimationAsset(uid);
    }

protected:
    bool importNative(const std::filesystem::path&, AnimationAsset*) override
    {
        return false;
    }

    uint64_t saveTyped(const AnimationAsset* source, uint8_t** outBuffer) override;
    void loadTyped(const uint8_t* buffer, AnimationAsset* dst) override;
};
