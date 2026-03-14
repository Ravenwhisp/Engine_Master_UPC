#pragma once

#include "ImporterNative.h"
#include <ModuleResources.h>
#include "UtilityGLFT.h"

class MeshAsset;

constexpr const char* MESH_EXTENSION = ".asset";

class ImporterMesh : public ImporterNative<MeshAsset, AssetType::MESH>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == MESH_EXTENSION;
    }

    Asset* createAssetInstance(const MD5Hash& uid) const override;

protected:
    bool     importNative(const std::filesystem::path& path, MeshAsset* dst)      override;
    uint64_t saveTyped(const MeshAsset* source, uint8_t** outBuffer)           override;
    void     loadTyped(const uint8_t* buffer, MeshAsset* dst)                override;
};