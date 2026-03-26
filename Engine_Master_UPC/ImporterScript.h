#pragma once
#include "ImporterNative.h"
#include "ScriptAsset.h"


class ImporterScript : public ImporterNative<ScriptAsset, AssetType::SCRIPT>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        return path.extension().string() == SCRIPT_EXTENSION;
    }

    Asset* createAssetInstance(const MD5Hash& uid) const override;
protected:
    bool     importNative(const std::filesystem::path& path, ScriptAsset* dst) override;
    uint64_t saveTyped(const ScriptAsset* source, uint8_t** outBuffer)      override;
    void     loadTyped(const uint8_t* buffer, ScriptAsset* dst)       override;
};