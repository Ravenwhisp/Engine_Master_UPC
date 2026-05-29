#include "Globals.h"
#include "TextureImportSettings.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

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

static bool iequals(std::string_view a, std::string_view b)
{
    return a.size() == b.size()
        && std::equal(a.begin(), a.end(), b.begin(),
            [](char ca, char cb) { return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb)); });
}

TextureImportFormat TextureImportSettings::DetectFromFilename(const std::filesystem::path& path)
{
    std::string stem = path.stem().string();

    std::vector<std::string> tokens;
    size_t start = 0;
    for (size_t i = 0; i <= stem.size(); ++i)
    {
        if (i == stem.size() || stem[i] == '_')
        {
            if (i > start)
            {
                tokens.push_back(stem.substr(start, i - start));
            }
            start = i + 1;
        }
    }

    for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
    {
        if (iequals(*it, "N") || iequals(*it, "Normal") || iequals(*it, "norm"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM;
        }

        if (iequals(*it, "H") || iequals(*it, "Height") || iequals(*it, "height")
            || iequals(*it, "Displacement") || iequals(*it, "Disp"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM;
        }

        if (iequals(*it, "M") || iequals(*it, "Mask") || iequals(*it, "mask"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM;
        }

        if (iequals(*it, "AO") || iequals(*it, "Occlusion") || iequals(*it, "AmbientOcclusion"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM;
        }

        if (iequals(*it, "R") || iequals(*it, "Roughness") || iequals(*it, "rough"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM;
        }

        if (iequals(*it, "Metallic") || iequals(*it, "Metalness") || iequals(*it, "metal"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM;
        }

        if (iequals(*it, "S") || iequals(*it, "Specular") || iequals(*it, "Spec") || iequals(*it, "spec"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM;
        }

        if (iequals(*it, "D") || iequals(*it, "Diffuse") || iequals(*it, "diff")
            || iequals(*it, "Albedo") || iequals(*it, "BaseColor") || iequals(*it, "C")
            || iequals(*it, "Col") || iequals(*it, "Color"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM_SRGB;
        }

        if (iequals(*it, "E") || iequals(*it, "Emissive") || iequals(*it, "Emission") || iequals(*it, "emis"))
        {
            return TextureImportFormat::R8G8B8A8_UNORM_SRGB;
        }
    }

    return TextureImportFormat::AUTO;
}
