#pragma once
#include <filesystem>
#include "MD5Fwd.h"
#include "AssetType.h"
#include "Extensions.h"
#include <CerealUtils.h>
#include "UID.h"

class Asset;

class Importer
{
public:
    virtual ~Importer() = default;

    virtual bool canImport(const std::filesystem::path& path) const = 0;
    virtual Asset* createAssetInstance(const UID& uid) const = 0;
    virtual AssetType getAssetType() const = 0;

    virtual bool     import(const std::filesystem::path& sourcePath, Asset* outAsset) = 0;
    virtual uint64_t save( const Asset* asset, uint8_t** outBuffer) = 0;
    virtual void     load(const uint8_t* buffer, uint64_t size, const UID& localId, Asset* outAsset) = 0;

    virtual bool canSaveToSource() const { return false; }
    virtual bool saveToSource(const std::filesystem::path& path, const Asset* asset) { return false; }
};