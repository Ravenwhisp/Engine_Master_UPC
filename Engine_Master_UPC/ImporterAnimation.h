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

    Asset* createAssetInstance(AssetReference& uid) const override
    {
        return new AnimationAsset(uid);
    }    
    
    bool saveNative(const AnimationAsset* asset, const std::filesystem::path& path);


protected:
    bool importNative(const std::filesystem::path&, AnimationAsset*) override
    {
        return false;
    }


};
