#pragma once
#include <filesystem>
#include <cstdint>
#include <vector>

class Importer
{
public:
    virtual ~Importer() = default;

    virtual bool canImport(const std::filesystem::path& path) const = 0;

    virtual bool import(const std::filesystem::path& sourcePath, void* outAsset ) = 0;
    virtual uint64_t save( const void* asset, uint8_t** outBuffer) = 0;
    virtual void load(const uint8_t* buffer, void* outAsset) = 0;
};