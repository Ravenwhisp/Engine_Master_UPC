#include "Globals.h"
#include "TextureAsset.h"
#include "TextureImportSettings.h"

#include "imgui.h"

#include "Application.h"
#include "ModuleRender.h"

namespace
{
    const char* FormatToString(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_UNKNOWN: return "UNKNOWN";
        case DXGI_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return "B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_BC1_UNORM: return "BC1_UNORM";
        case DXGI_FORMAT_BC1_UNORM_SRGB: return "BC1_UNORM_SRGB";
        case DXGI_FORMAT_BC2_UNORM: return "BC2_UNORM";
        case DXGI_FORMAT_BC2_UNORM_SRGB: return "BC2_UNORM_SRGB";
        case DXGI_FORMAT_BC3_UNORM: return "BC3_UNORM";
        case DXGI_FORMAT_BC3_UNORM_SRGB: return "BC3_UNORM_SRGB";
        case DXGI_FORMAT_BC4_UNORM: return "BC4_UNORM";
        case DXGI_FORMAT_BC5_UNORM: return "BC5_UNORM";
        case DXGI_FORMAT_BC7_UNORM: return "BC7_UNORM";
        case DXGI_FORMAT_BC7_UNORM_SRGB: return "BC7_UNORM_SRGB";
        default: return "Other";
        }
    }
}

std::unique_ptr<ImportSettings> TextureAsset::createDefaultImportSettings() const
{
    return std::make_unique<TextureImportSettings>();
}

void TextureAsset::drawUI()
{
    ImGui::Text("Texture Asset");
    ImGui::Separator();

    ImGui::Text("UID: %llu", static_cast<unsigned long long>(getUID()));
    ImGui::Text("Size: %u x %u", width, height);
    ImGui::Text("Format: %s", FormatToString(format));
    ImGui::Text("Mip Count: %u", mipCount);
    ImGui::Text("Array Size: %u", arraySize);
    ImGui::Text("Image Count: %u", imageCount);

    ImGui::Spacing();

    if (m_importSettings)
    {
        ImGui::Separator();
        ImGui::Text("Import Settings");
        ImGui::Indent();
        m_importSettings->drawUI();
        ImGui::Unindent();
    }

    ImGui::Spacing();

    if (images.empty())
    {
        ImGui::TextDisabled("No CPU image data available.");
        return;
    }

    if (ImGui::TreeNode("Images"))
    {
        for (size_t i = 0; i < images.size(); ++i)
        {
            const TextureImage& image = images[i];

            ImGui::PushID(static_cast<int>(i));

            std::string label = "Image " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str()))
            {
                ImGui::Text("Row Pitch: %u", image.rowPitch);
                ImGui::Text("Slice Pitch: %u", image.slicePitch);
                ImGui::Text("Pixel Data Size: %zu bytes", image.pixels.size());

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

