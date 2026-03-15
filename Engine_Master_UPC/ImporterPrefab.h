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

    Asset* createAssetInstance(const MD5Hash& uid) const override;

protected:
    bool     importNative(const std::filesystem::path& path, PrefabAsset* dst) override;
    uint64_t saveTyped(const PrefabAsset* source, uint8_t** outBuffer)      override;
    void     loadTyped(const uint8_t* buffer, PrefabAsset* dst)         override;
};