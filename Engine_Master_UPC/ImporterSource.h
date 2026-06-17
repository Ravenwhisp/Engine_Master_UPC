#pragma once
#include "Importer.h"
#include "BinaryArchive.h"
#include <cstring>

template<typename ExternalFormat, typename AssetFormat, AssetType TType>
class ImporterSource : public Importer
{
public:
    bool import(const std::filesystem::path& path, Asset* outAsset) override
    {
        ExternalFormat external{};
        if (!loadExternal(path, external))
        {
            DEBUG_ERROR("Error while trying to load the asset from this path:", path.c_str());
            return false;
        }

        importTyped(external, static_cast<AssetFormat*>(outAsset));

        return true;
    }

    AssetType getAssetType() const override
    {
        return TType;
    }

    uint64_t save(const Asset* asset, uint8_t** outBuffer) final
    {
        BinaryArchive archive(ArchiveMode::Output);
        const_cast<AssetFormat*>(static_cast<const AssetFormat*>(asset))->serialize(archive);
        const size_t sz = archive.size();
        uint8_t* buf = new uint8_t[sz];
        std::memcpy(buf, archive.data(), sz);
        *outBuffer = buf;
        return sz;
    }

    void load(const uint8_t* buffer, Asset* outAsset) final
    {
        BinaryArchive archive(buffer, ArchiveMode::Input);
        static_cast<AssetFormat*>(outAsset)->serialize(archive);
    }

    bool saveNative(const Asset* asset, const std::filesystem::path& path) final
	{
        return false;
	}

protected:
    virtual bool loadExternal(const std::filesystem::path& path,ExternalFormat& out) = 0;

    virtual void importTyped(const ExternalFormat& source, AssetFormat* dst) = 0;
};