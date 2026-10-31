#include "Globals.h"
#include "ModuleFont.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "FontAsset.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "DescriptorHeap.h"

ModuleFont::ModuleFont() = default;
ModuleFont::~ModuleFont() = default;

bool ModuleFont::init()
{
    return true;
}

bool ModuleFont::loadFonts(ID3D12Device* device, ResourceUploadBatch& upload, DescriptorHeap& fontHeap)
{
    m_spriteFonts.clear();
    m_fontNames.clear();

    std::vector<std::pair<std::string, AssetId>> fonts;
#ifdef GAME_RELEASE
    // The build config preserves font names used by UIText and their Library references.
    // FontPass is initialized before Application parses its scene build settings.
    std::ifstream config("build.cfg");
    std::string line;
    while (std::getline(config, line))
    {
        std::istringstream record(line);
        std::string kind;
        UID uid = INVALID_UID;
        std::string hash;
        std::string name;
        if (record >> kind && kind == "font" && record >> uid >> hash
            && std::getline(record >> std::ws, name) && !name.empty())
        {
            if (!name.empty() && name.back() == '\r')
                name.pop_back();
            AssetId ref(uid, hash, AssetType::FONT);
            if (ref.isValid() && !name.empty())
                fonts.emplace_back(std::move(name), ref);
        }
    }
#else
    for (const auto& [uid, entry] : app->getModuleAssets()->getIndex().allEntries())
    {
        if (entry.type == AssetType::FONT && isValidAsset(entry.contentHash))
            fonts.emplace_back(entry.sourcePath.stem().string(), AssetId(uid, entry.contentHash, AssetType::FONT));
    }
#endif
    std::sort(fonts.begin(), fonts.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (auto& [name, ref] : fonts)
    {
        if (m_spriteFonts.size() >= fontHeap.capacity())
        {
            DEBUG_ERROR("[ModuleFont] Font descriptor capacity exceeded.");
            break;
        }
        auto asset = app->getModuleAssets()->load<FontAsset>(ref);
        if (!asset || asset->getSpriteFontData().empty())
        {
            DEBUG_ERROR("[ModuleFont] Failed to load font '%s' from Library.", name.c_str());
            continue;
        }

        const auto& data = asset->getSpriteFontData();
        const auto index = static_cast<UINT>(m_spriteFonts.size());
        m_spriteFonts.emplace_back(std::make_unique<SpriteFont>(
            device, upload, data.data(), data.size(),
            fontHeap.getCPUHandle(index), fontHeap.getGPUHandle(index)));
        m_fontNames.push_back(name);
        DEBUG_LOG("Loaded font: %s", name.c_str());
    }

    if (m_spriteFonts.empty())
        DEBUG_ERROR("[ModuleFont] No fonts loaded. Export the build config with font references and include their Library assets.");
    return !m_spriteFonts.empty();
}

SpriteFont* ModuleFont::getFont(int fontId) const
{
    if (fontId < 0 || fontId >= static_cast<int>(m_spriteFonts.size()))
        return nullptr;

    return m_spriteFonts[fontId].get();
}

const SpriteFont::Glyph* ModuleFont::getGlyph(int fontId, wchar_t character) const
{
    SpriteFont* font = getFont(fontId);

    if (!font)
        return nullptr;

    if (!font->ContainsCharacter(character))
    {
        const wchar_t defaultCharacter = font->GetDefaultCharacter();

        if (defaultCharacter == 0 || !font->ContainsCharacter(defaultCharacter))
            return nullptr;

        character = defaultCharacter;
    }

    return font->FindGlyph(character);
}

D3D12_GPU_DESCRIPTOR_HANDLE ModuleFont::getFontTexture(int fontId) const
{
    SpriteFont* font = getFont(fontId);

    if (!font)
        return {};

    return font->GetSpriteSheet();
}

DirectX::XMUINT2 ModuleFont::getFontTextureSize(int fontId) const
{
    SpriteFont* font = getFont(fontId);

    if (!font)
        return DirectX::XMUINT2(0, 0);

    return font->GetSpriteSheetSize();
}

float ModuleFont::getLineSpacing(int fontId) const
{
    SpriteFont* font = getFont(fontId);

    if (!font)
        return 0.0f;

    return font->GetLineSpacing();
}

int ModuleFont::findFontId(const std::string& fontName) const
{
    for (size_t i = 0; i < m_fontNames.size(); ++i)
    {
        if (m_fontNames[i] == fontName)
            return static_cast<int>(i);
    }

    return INVALID_FONT_ID;
}