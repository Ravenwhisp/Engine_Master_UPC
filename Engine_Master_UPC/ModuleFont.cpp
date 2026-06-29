#include "Globals.h"
#include "ModuleFont.h"

#include <filesystem>

#include "DescriptorHeap.h"

ModuleFont::ModuleFont() = default;
ModuleFont::~ModuleFont() = default;

bool ModuleFont::init()
{
    return true;
}

bool ModuleFont::loadFonts(ID3D12Device* device, ResourceUploadBatch& upload, DescriptorHeap& fontHeap)
{
    namespace fs = std::filesystem;

    const fs::path fontsFolder = L"Assets/Fonts";

    m_spriteFonts.clear();
    m_fontNames.clear();

    if (!fs::exists(fontsFolder))
        return false;

    for (const auto& entry : fs::directory_iterator(fontsFolder))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() != L".spritefont")
            continue;

        const auto index = static_cast<UINT>(m_spriteFonts.size());

        m_spriteFonts.emplace_back(std::make_unique<SpriteFont>(
            device,
            upload,
            entry.path().c_str(),
            fontHeap.getCPUHandle(index),
            fontHeap.getGPUHandle(index)
        ));

        m_fontNames.push_back(entry.path().stem().string());
        DEBUG_LOG("Loaded font: %s", entry.path().stem().string().c_str());
    }

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