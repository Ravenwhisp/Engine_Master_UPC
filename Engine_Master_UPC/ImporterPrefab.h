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

    Asset* createAssetInstance(const UID& uid) const override;

protected:
    bool     importNative(const std::filesystem::path& path, PrefabAsset* dst) override;
    bool     saveNative(const std::filesystem::path& path, const PrefabAsset* src) override;
    uint64_t saveTyped(const PrefabAsset* source, uint8_t** outBuffer)      override;
    void     loadTyped(const uint8_t* buffer, PrefabAsset* dst)         override;
};