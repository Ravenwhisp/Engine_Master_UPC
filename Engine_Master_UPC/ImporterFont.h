#pragma once
#include "ImporterSource.h"
#include "FontAsset.h"

#include <filesystem>
#include <string>

constexpr const char* TTL_EXTENSION = ".ttf";


class ImporterFont : public ImporterSource<std::vector<uint8_t>, FontAsset, AssetType::FONT>
{
public:
    bool canImport(const std::filesystem::path& path) const override
    {
        auto ext = path.extension().string();
        return ext == TTL_EXTENSION;
    }

    Asset* createAssetInstance(const MD5Hash& uid) const override;

protected:
    bool loadExternal(const std::filesystem::path& path, std::vector<uint8_t>& out) override;

    void  importTyped(const std::vector<uint8_t>& source, FontAsset* dst) override;

    uint64_t saveTyped(const FontAsset* source, uint8_t** buffer) override;

    void  loadTyped(const uint8_t* buffer, FontAsset* dst) override;

private:
    std::string m_lastFontFamilyName;

    std::filesystem::path runMakeSpriteFont(const std::filesystem::path& ttfPath);
};