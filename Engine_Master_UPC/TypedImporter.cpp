#pragma once
#include "Importer.h"

template<typename ExternalFormat, typename AssetFormat>
class TypedImporter : public Importer
{
public:
    bool import( const std::filesystem::path& path,void* outAsset) final
    {
        ExternalFormat external{};
        if (!loadExternal(path, external)) return false;

        importTyped(external, static_cast<AssetFormat*>(outAsset));
        return true;
    }

    uint64_t save(const void* asset, uint8_t** outBuffer) final
    {
        return saveTyped(static_cast<const AssetFormat*>(asset), outBuffer);
    }

    void load(const uint8_t* buffer, void* outAsset) final
    {
        loadTyped( buffer, static_cast<AssetFormat*>(outAsset));
    }

protected:
    virtual bool loadExternal(const std::filesystem::path& path, ExternalFormat& out) = 0;

    virtual void importTyped(const ExternalFormat& src, AssetFormat* dst) = 0;

    virtual uint64_t saveTyped(const AssetFormat* src, uint8_t** buffer) = 0;

    virtual void loadTyped( const uint8_t* buffer, AssetFormat* dst) = 0;
};