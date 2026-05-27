#pragma once
#include "Importer.h"
#include "BinaryArchive.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/filereadstream.h"
#include <rapidjson/writer.h>
#include <fstream>
#include <cstring>

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
        BinaryArchive archive(ArchiveMode::Output);
        const_cast<AssetFormat*>(static_cast<const AssetFormat*>(asset))->serialize(archive);
        const size_t sz = archive.size();
        uint8_t* buf = new uint8_t[sz];
        std::memcpy(buf, archive.data(), sz);
        *outBuffer = buf;
        return sz;
    }

    void load(const uint8_t* buffer, Asset* outAsset) override
    {
        BinaryArchive archive(buffer, ArchiveMode::Input);
        static_cast<AssetFormat*>(outAsset)->serialize(archive);
    }

    bool saveNative(const Asset* asset, const std::filesystem::path& path) override
    {
        return saveNative(static_cast<const AssetFormat*>(asset), path);
    }

protected:
    virtual bool     saveNative(const AssetFormat* asset, const std::filesystem::path& path) = 0;
    virtual bool     importNative(const std::filesystem::path& path, AssetFormat* dst) = 0;
};