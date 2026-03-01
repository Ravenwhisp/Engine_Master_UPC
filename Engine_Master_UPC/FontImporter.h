#pragma once
#include "TypedImporter.h"
#include <filesystem>
#include <string>


struct FontAsset : public Asset
{
public:
    friend class FontImporter;

    FontAsset(UID id) : Asset(id, AssetType::FONT) {}
private:
    std::string fontFamilyName;
    std::vector<uint8_t> spriteFontData;
};

constexpr const char* TTL_EXTENSION = ".ttf";


class FontImporter : public TypedImporter<std::vector<uint8_t>, FontAsset>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        auto ext = path.extension().string();
        return ext == TTL_EXTENSION;
    }

    Asset* createAssetInstance(UID uid) const override
    {
        return new FontAsset(uid);
    }

protected:
    bool loadExternal(const std::filesystem::path& path, std::vector<uint8_t>& out) override;

    void  importTyped(const std::vector<uint8_t>& source, FontAsset* dst) override;

    uint64_t saveTyped(const FontAsset* source, uint8_t** buffer) override;

    void  loadTyped(const uint8_t* buffer, FontAsset* dst) override;

private:
    std::string m_lastFontFamilyName;

    std::filesystem::path runMakeSpriteFont(const std::filesystem::path& ttfPath);
};