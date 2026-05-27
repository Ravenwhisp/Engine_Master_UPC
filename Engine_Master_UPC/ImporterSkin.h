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

    Asset* createAssetInstance(AssetReference& uid) const override
    {
        return new SkinAsset(uid);
    }
    bool saveNative(const SkinAsset* asset, const std::filesystem::path& path);

protected:
    bool importNative(const std::filesystem::path&, SkinAsset*) override
    {
        return false;
    }


};
