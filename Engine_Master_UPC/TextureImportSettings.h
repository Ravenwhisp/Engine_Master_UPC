#pragma once
#include "ImportSettings.h"
#include <cstdint>
#include <cstring>
#include <filesystem>

enum class TextureImportFormat : uint32_t
{
    AUTO = 0,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    BC1_UNORM,
    BC3_UNORM,
    BC5_UNORM,
    BC7_UNORM,
    BC7_UNORM_SRGB
};

inline const char* TextureImportFormatToString(uint32_t v)
{
    switch (static_cast<TextureImportFormat>(v))
    {
    case TextureImportFormat::AUTO:               return "AUTO";
    case TextureImportFormat::R8G8B8A8_UNORM:     return "R8G8B8A8_UNORM";
    case TextureImportFormat::R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_UNORM_SRGB";
    case TextureImportFormat::BC1_UNORM:          return "BC1_UNORM";
    case TextureImportFormat::BC3_UNORM:          return "BC3_UNORM";
    case TextureImportFormat::BC5_UNORM:          return "BC5_UNORM";
    case TextureImportFormat::BC7_UNORM:          return "BC7_UNORM";
    case TextureImportFormat::BC7_UNORM_SRGB:     return "BC7_UNORM_SRGB";
    default: return "AUTO";
    }
}

inline uint32_t StringToTextureImportFormat(const char* s)
{
    if (std::strcmp(s, "AUTO") == 0)                return 0;
    if (std::strcmp(s, "R8G8B8A8_UNORM") == 0)     return 1;
    if (std::strcmp(s, "R8G8B8A8_UNORM_SRGB") == 0) return 2;
    if (std::strcmp(s, "BC1_UNORM") == 0)           return 3;
    if (std::strcmp(s, "BC3_UNORM") == 0)           return 4;
    if (std::strcmp(s, "BC5_UNORM") == 0)           return 5;
    if (std::strcmp(s, "BC7_UNORM") == 0)           return 6;
    if (std::strcmp(s, "BC7_UNORM_SRGB") == 0)      return 7;
    return 0;
}

class TextureImportSettings : public ImportSettings
{
public:
    TextureImportFormat targetFormat = TextureImportFormat::AUTO;
    bool generateMips = true;
    bool srgb = true;

    // Transient — not serialized; set during import from filename detection when targetFormat is AUTO
    TextureImportFormat resolvedFormat = TextureImportFormat::AUTO;

    void serialize(IArchive& archive) override;
    std::unique_ptr<ImportSettings> clone() const override;
    const char* getTypeName() const override { return "TextureImportSettings"; }
    void drawUI() override;

    static TextureImportFormat DetectFromFilename(const std::filesystem::path& path);
};
