#include "Globals.h"
#include "TextureImportSettings.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "imgui.h"

static const char* TextureImportFormatToString(TextureImportFormat fmt)
{
    switch (fmt)
    {
    case TextureImportFormat::AUTO:               return "AUTO";
    case TextureImportFormat::R8G8B8A8_UNORM:     return "R8G8B8A8_UNORM";
    case TextureImportFormat::R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_UNORM_SRGB";
    case TextureImportFormat::BC1_UNORM:          return "BC1_UNORM (DXT1)";
    case TextureImportFormat::BC3_UNORM:          return "BC3_UNORM (DXT5)";
    case TextureImportFormat::BC5_UNORM:          return "BC5_UNORM (Normal)";
    case TextureImportFormat::BC7_UNORM:          return "BC7_UNORM";
    case TextureImportFormat::BC7_UNORM_SRGB:     return "BC7_UNORM_SRGB";
    default:                                      return "Unknown";
    }
}

void TextureImportSettings::save(rapidjson::Value& obj, rapidjson::Document::AllocatorType& allocator) const
{
    obj.AddMember("targetFormat", static_cast<uint32_t>(targetFormat), allocator);
    obj.AddMember("generateMips", generateMips, allocator);
    obj.AddMember("srgb", srgb, allocator);
}

void TextureImportSettings::load(const rapidjson::Value& obj)
{
    if (obj.HasMember("targetFormat") && obj["targetFormat"].IsUint())
        targetFormat = static_cast<TextureImportFormat>(obj["targetFormat"].GetUint());
    if (obj.HasMember("generateMips") && obj["generateMips"].IsBool())
        generateMips = obj["generateMips"].GetBool();
    if (obj.HasMember("srgb") && obj["srgb"].IsBool())
        srgb = obj["srgb"].GetBool();
}

std::unique_ptr<ImportSettings> TextureImportSettings::clone() const
{
    auto c = std::make_unique<TextureImportSettings>();
    c->targetFormat = targetFormat;
    c->generateMips = generateMips;
    c->srgb = srgb;
    return c;
}

void TextureImportSettings::drawUI()
{
    const char* preview = TextureImportFormatToString(targetFormat);
    if (ImGui::BeginCombo("Format", preview))
    {
        for (uint32_t i = 0; i <= static_cast<uint32_t>(TextureImportFormat::BC7_UNORM_SRGB); ++i)
        {
            auto fmt = static_cast<TextureImportFormat>(i);
            bool isSelected = (targetFormat == fmt);
            if (ImGui::Selectable(TextureImportFormatToString(fmt), isSelected))
                targetFormat = fmt;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Checkbox("Generate Mip Maps", &generateMips);

    if (targetFormat == TextureImportFormat::AUTO)
        ImGui::Checkbox("sRGB (Color Data)", &srgb);
}
