#pragma once
#include "ImporterNative.h"

#include "Prefab.h"

class ImporterPrefab : public ImporterNative<Prefab, AssetType::PREFAB>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == PREFAB_EXTENSION;
    }

    Asset* createAssetInstance(AssetReference& uid) const override;
    bool saveNative(const Prefab* asset, const std::filesystem::path& path);
protected:
    bool     importNative(const std::filesystem::path& path, Prefab* dst) override;
};