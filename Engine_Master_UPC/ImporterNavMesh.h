#pragma once
#include "Importer.h"

class NavMeshAsset;

class ImporterNavMesh : public Importer
{
public:
    bool canImport(const std::filesystem::path& path) const override;
    Asset* createAssetInstance(AssetId& uid) const override;
    AssetType getAssetType() const override;

    bool saveNative(const Asset* asset, const std::filesystem::path& path) override;
    bool import(const std::filesystem::path& sourcePath, Asset* outAsset) override;
    uint64_t save(const Asset* asset, uint8_t** outBuffer) override;
    void load(const uint8_t* buffer, Asset* outAsset) override;
};
