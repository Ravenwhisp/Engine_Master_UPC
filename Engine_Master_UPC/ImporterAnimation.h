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

    Asset* createAssetInstance(const UID& uid) const override
    {
        return new AnimationAsset(uid);
    }    
    
    bool saveNative(const AnimationAsset* asset, const std::filesystem::path& path);


protected:
    bool importNative(const std::filesystem::path&, AnimationAsset*) override
    {
        return false;
    }

    uint64_t saveTyped(const AnimationAsset* source, uint8_t** outBuffer) override;
    void loadTyped(const uint8_t* buffer, AnimationAsset* dst) override;
};
