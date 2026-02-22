#pragma once
#include <filesystem>
#include <cstdint>
#include <vector>
#include "Asset.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"


class Importer
{
public:
    virtual ~Importer() = default;

    virtual bool canImport(const std::filesystem::path& path) const = 0;
    virtual Asset* createAssetInstance(UID uid) const = 0;

    virtual bool import(const std::filesystem::path& sourcePath, Asset * outAsset ) = 0;
    virtual uint64_t save( const Asset* asset, uint8_t** outBuffer) = 0;
    virtual void load(const uint8_t* buffer, Asset* outAsset) = 0;
};