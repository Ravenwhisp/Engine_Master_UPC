#pragma once
#include "ImporterNative.h"
#include "MeshAsset.h"

class ImporterMesh : public ImporterNative<MeshAsset, AssetType::MESH>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == MESH_EXTENSION;
    }

    Asset* createAssetInstance(AssetReference& uid) const override;

    bool saveNative(const MeshAsset* asset, const std::filesystem::path& path);

protected:
    bool     importNative(const std::filesystem::path& path, MeshAsset* dst)      override;
};