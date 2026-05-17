#pragma once
#include <filesystem>
#include "AssetReference.h"
#include "AssetType.h"
#include "Extensions.h"

class Asset;

class Importer
{
public:
    virtual ~Importer() = default;

    virtual bool canImport(const std::filesystem::path& path) const = 0;
    virtual Asset* createAssetInstance(AssetReference& uid) const = 0;
    virtual AssetType getAssetType() const = 0;

    virtual bool saveNative(const Asset* asset, const std::filesystem::path& path) = 0;
    virtual bool import(const std::filesystem::path& sourcePath, Asset* outAsset) = 0;
    virtual uint64_t save(const Asset* asset, uint8_t** outBuffer) = 0;
    virtual void load(const uint8_t* buffer, Asset* outAsset) = 0;
};