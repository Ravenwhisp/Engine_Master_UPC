#pragma once
#include "ImporterNative.h"
#include "MaterialAsset.h"


// Importer for the engine's own .material format.
class ImporterMaterial : public ImporterNative<MaterialAsset, AssetType::MATERIAL>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == MATERIAL_EXTENSION;
    }

    Asset* createAssetInstance(AssetReference& uid) const override;

    bool saveNative(const MaterialAsset* asset, const std::filesystem::path& path);
protected:
    bool     importNative(const std::filesystem::path& path, MaterialAsset* dst) override;
};