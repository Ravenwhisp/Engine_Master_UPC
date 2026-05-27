#pragma once
#include "ImporterNative.h"

#include "PrefabAsset.h"

class ImporterPrefab : public ImporterNative<PrefabAsset, AssetType::PREFAB>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == PREFAB_EXTENSION;
    }

    Asset* createAssetInstance(AssetReference& uid) const override;
    bool saveNative(const PrefabAsset* asset, const std::filesystem::path& path);
protected:
    bool     importNative(const std::filesystem::path& path, PrefabAsset* dst) override;
};