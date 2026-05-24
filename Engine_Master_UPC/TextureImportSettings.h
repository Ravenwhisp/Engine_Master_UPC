#pragma once
#include "ImportSettings.h"
#include <cstdint>

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

class TextureImportSettings : public ImportSettings
{
public:
    TextureImportFormat targetFormat = TextureImportFormat::AUTO;
    bool generateMips = true;
    bool srgb = true;

    void save(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const override;
    void load(const rapidjson::Value& obj) override;
    std::unique_ptr<ImportSettings> clone() const override;
    const char* getTypeName() const override { return "TextureImportSettings"; }
    void drawUI() override;
};
