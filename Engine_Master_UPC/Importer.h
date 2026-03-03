#pragma once
#include <filesystem>
#include <cstdint>
#include <vector>
#include "Asset.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "Globals.h"

class Importer
{
public:
    virtual ~Importer() = default;

    virtual bool canImport(const std::filesystem::path& path) const = 0;
    virtual std::shared_ptr<Asset> createAssetInstance(UID uid) const = 0;

    virtual bool import(const std::filesystem::path& sourcePath, std::shared_ptr<Asset> outAsset ) = 0;
    virtual uint64_t save( const std::shared_ptr<Asset> asset, uint8_t** outBuffer) = 0;
    virtual void load(const uint8_t* buffer, std::shared_ptr<Asset> outAsset) = 0;
};