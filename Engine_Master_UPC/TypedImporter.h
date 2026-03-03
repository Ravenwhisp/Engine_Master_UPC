#pragma once
#include "Importer.h"

template<typename ExternalFormat, typename AssetFormat>
class TypedImporter : public Importer
{
public:
    bool import(const std::filesystem::path& path, std::shared_ptr<Asset> outAsset) final
    {
        ExternalFormat external{};
        if (!loadExternal(path, external))
        {
            DEBUG_ERROR("Error while trying to load the asset from this path:", path.c_str());
            return false;
        }

        importTyped(external, std::static_pointer_cast<AssetFormat>(outAsset));

        return true;
    }

    uint64_t save(const std::shared_ptr<Asset> asset, uint8_t** outBuffer) final
    {
        return saveTyped(std::static_pointer_cast<AssetFormat>(asset), outBuffer);
    }

    void load(const uint8_t* buffer, std::shared_ptr<Asset> outAsset) final
    {
        loadTyped(buffer, std::static_pointer_cast<AssetFormat>(outAsset));
    }

protected:
    virtual bool loadExternal(const std::filesystem::path& path,ExternalFormat& out) = 0;

    virtual void importTyped(const ExternalFormat& source, std::shared_ptr<AssetFormat> dst) = 0;

    virtual uint64_t saveTyped(const std::shared_ptr<AssetFormat> source, uint8_t** buffer) = 0;

    virtual void loadTyped( const uint8_t* buffer, std::shared_ptr<AssetFormat> dst) = 0;
};