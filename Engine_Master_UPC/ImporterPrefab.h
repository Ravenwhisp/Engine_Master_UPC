#pragma once
#include "ImporterNative.h"

#include "Prefab.h"

class ImporterPrefab : public ImporterNative<Prefab, AssetType::PREFAB>
{
public:
    ImporterPrefab()
        : ImporterNative({PREFAB_EXTENSION})
    {
    }

protected:
    bool saveNativeFile(const Prefab* asset, const std::filesystem::path& path) override;
    bool importNative(const std::filesystem::path& path, Prefab* dst) override;
};
