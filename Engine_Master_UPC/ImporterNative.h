#pragma once
#include "Importer.h"


template<typename AssetFormat, AssetType TType>
class ImporterNative : public Importer
{
public:
    AssetType getAssetType() const override
    {
        return TType;
    }

    bool import(const std::filesystem::path & path, Asset * outAsset) final
    {
        return importNative(path, static_cast<AssetFormat*>(outAsset));
    }

    uint64_t save(const Asset* asset, uint8_t** outBuffer) override
    {
        return saveTyped(static_cast<const AssetFormat*>(asset), outBuffer);
    }

    void load(const uint8_t* buffer, Asset* outAsset) override
    {
        loadTyped(buffer, static_cast<AssetFormat*>(outAsset));
    }

    bool saveNative(const Asset* asset, const std::filesystem::path& path) override
    {
        return saveNative(static_cast<const AssetFormat*>(asset), path);
    }

protected:
    virtual bool     saveNative(const AssetFormat* asset, const std::filesystem::path& path) = 0;
    virtual bool     importNative(const std::filesystem::path& path, AssetFormat* dst) = 0;
    virtual uint64_t saveTyped(const AssetFormat* source, uint8_t** outBuffer) = 0;
    virtual void     loadTyped(const uint8_t* buffer, AssetFormat* dst) = 0;
};