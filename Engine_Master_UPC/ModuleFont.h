#pragma once

#include "Module.h"

#include <memory>
#include <vector>
#include <string>

#include "SpriteFont.h"
#include "ResourceUploadBatch.h"

class DescriptorHeap;

static constexpr int INVALID_FONT_ID = -1;
static constexpr int UNKNOWN_FONT_ID = -2;

class ModuleFont : public Module
{
private:
	std::vector<std::unique_ptr<SpriteFont>> m_spriteFonts;
	std::vector<std::string> m_fontNames;

public:
	ModuleFont();
	~ModuleFont();

	bool init() override;

	bool loadFonts(ID3D12Device* device, ResourceUploadBatch& upload, DescriptorHeap& fontHeap);

	SpriteFont* getFont(int fontId) const;
	const SpriteFont::Glyph* getGlyph(int fontId, wchar_t character) const;
	D3D12_GPU_DESCRIPTOR_HANDLE getFontTexture(int fontId) const;
	DirectX::XMUINT2 getFontTextureSize(int fontId) const;
	float getLineSpacing(int fontId) const;

	int findFontId(const std::string& fontName) const;

	const std::vector<std::string>& getFontNames() const { return m_fontNames; }
	int getFontCount() const { return static_cast<int>(m_spriteFonts.size()); }
};