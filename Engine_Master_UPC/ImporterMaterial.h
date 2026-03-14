#pragma once
#include "ImporterNative.h"

class MaterialAsset;

constexpr const char* MATERIAL_EXTENSION = ".material";

// Importer for the engine's own .material format.
class ImporterMaterial : public ImporterNative<MaterialAsset, AssetType::MATERIAL>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == MATERIAL_EXTENSION;
    }

    Asset* createAssetInstance(const MD5Hash& uid) const override;
protected:
    bool     importNative(const std::filesystem::path& path, MaterialAsset* dst) override;
    uint64_t saveTyped(const MaterialAsset* source, uint8_t** outBuffer)      override;
    void     loadTyped(const uint8_t* buffer, MaterialAsset* dst)       override;
};