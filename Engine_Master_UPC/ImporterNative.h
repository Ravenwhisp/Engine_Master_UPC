#pragma once
#include "Importer.h"
#include "BinaryArchive.h"
#include "JsonArchive.h"

#include <vector>
#include <string>

template<typename AssetFormat, AssetType TType>
class ImporterNative : public Importer
{
public:
    ImporterNative(std::initializer_list<const char*> extensions)
    {
        for (const char* ext : extensions)
            m_extensions.push_back(ext);
    }

    AssetType getAssetType() const override
    {
        return TType;
    }

    bool canImport(const std::filesystem::path& path) const override
    {
        for (const auto& ext : m_extensions)
            if (path.extension() == ext) return true;
        return false;
    }

    Asset* createAssetInstance(AssetReference& uid) const override
    {
        return new AssetFormat(uid);
    }

    bool import(const std::filesystem::path& path, Asset* outAsset) final
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
        return saveNativeFile(static_cast<const AssetFormat*>(asset), path);
    }

protected:
    virtual bool saveNativeFile(const AssetFormat* asset, const std::filesystem::path& path)
    {
        JsonArchive archive(ArchiveMode::Output);
        const_cast<AssetFormat*>(asset)->serialize(archive);
        return archive.saveFile(path);
    }

    virtual bool importNative(const std::filesystem::path& path, AssetFormat* dst)
    {
        JsonArchive archive(ArchiveMode::Input);
        if (!archive.loadFile(path))
            return false;
        dst->serialize(archive);
        return true;
    }

private:
    std::vector<std::string> m_extensions;
};
