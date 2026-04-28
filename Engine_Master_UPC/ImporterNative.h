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

    bool import(const std::filesystem::path & path, Asset* outAsset) final
    {
        return importNative(path, static_cast<AssetFormat*>(outAsset));
    }

    bool canSaveToSource() const override { return true; }
    bool saveToSource(const std::filesystem::path& path, const Asset* asset) override
    {
        return saveNative(path, static_cast<const AssetFormat*>(asset));
    }

    uint64_t save(const Asset* asset, uint8_t** outBuffer) override
    {
        return saveTyped(static_cast<const AssetFormat*>(asset), outBuffer);
    }

    void load(const uint8_t* buffer, uint64_t size, const UID& localId, Asset* outAsset)
    {
        loadTyped(buffer, static_cast<AssetFormat*>(outAsset));
    }

protected:
    virtual bool     saveNative(const std::filesystem::path& path, const AssetFormat* src) = 0;
    virtual bool     importNative(const std::filesystem::path& path, AssetFormat* dst) = 0;

    virtual uint64_t saveTyped(const AssetFormat* source, uint8_t** outBuffer) = 0;
    virtual void     loadTyped(const uint8_t* buffer, AssetFormat* dst) = 0;
};