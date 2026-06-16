#pragma once
#include "Globals.h"
#include "Asset.h"
#include "IArchive.h"

class FontAsset : public Asset
{
public:
    friend class ImporterFont;

    FontAsset() {}
    FontAsset(AssetReference& id) : Asset(id, AssetType::FONT) {}

    void serialize(IArchive& archive) override
    {
        archive.serialize(fontFamilyName);

        uint64_t dataSize = static_cast<uint64_t>(spriteFontData.size());
        archive.serialize(dataSize);
        if (archive.mode() == ArchiveMode::Input)
            spriteFontData.resize(static_cast<size_t>(dataSize));
        archive.serializeRaw(spriteFontData.data(), static_cast<size_t>(dataSize));
    }

private:
    std::string fontFamilyName;
    std::vector<uint8_t> spriteFontData;
};