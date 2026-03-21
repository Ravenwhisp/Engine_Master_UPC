#pragma once
#include "Importer.h"

// Companion to SourceImporter<> for importers where the source file maps
// directly to the engine asset — no intermediate parsing step required.
//
// The template handles all Asset* ↔ AssetFmt* casts so subclasses work
// with concrete types throughout:
//
//   importDirect(path, AssetFmt*)   — read the source file into the asset
//   saveTyped   (const AssetFmt*)   — serialize to binary cache
//   loadTyped   (buffer, AssetFmt*) — deserialize from binary cache
//
// Example use cases: .material, .scene, any engine-native format that can
// be read in a single pass without an intermediate representation.
//
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

protected:
    virtual bool     importNative(const std::filesystem::path& path, AssetFormat* dst) = 0;
    virtual uint64_t saveTyped(const AssetFormat* source, uint8_t** outBuffer) = 0;
    virtual void     loadTyped(const uint8_t* buffer, AssetFormat* dst) = 0;
};