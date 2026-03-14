#pragma once
#include "Globals.h"
#include "Asset.h"

class FontAsset : public Asset
{
public:
    friend class ImporterFont;

    FontAsset() {}
    FontAsset(MD5Hash id) : Asset(id, AssetType::FONT) {}
private:
    std::string fontFamilyName;
    std::vector<uint8_t> spriteFontData;
};