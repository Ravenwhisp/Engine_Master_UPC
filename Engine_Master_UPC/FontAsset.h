#pragma once
#include "Asset.h"

struct FontAsset : public Asset
{
public:
    friend class ImporterFont;

    FontAsset(MD5Hash id) : Asset(id, AssetType::FONT) {}
private:
    std::string fontFamilyName;
    std::vector<uint8_t> spriteFontData;
};