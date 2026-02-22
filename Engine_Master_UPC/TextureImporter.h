#pragma once
#include "TypedImporter.h"
#include <DirectXTex.h>

#include "TextureAsset.h"

constexpr const char* PNG_EXTENSION = ".png";
constexpr const char* JPG_EXTENSION = ".jpg";
constexpr const char* JPEG_EXTENSION = ".jpeg";
constexpr const char* BMP_EXTENSION = ".bmp";
constexpr const char* TGA_EXTENSION = ".tga";
constexpr const char* DDS_EXTENSION = ".dds";

class TextureImporter : public TypedImporter<DirectX::ScratchImage, TextureAsset>
{
public:

    bool canImport(const std::filesystem::path& path) const override
	{
		auto ext = path.extension().string();
		return ext == PNG_EXTENSION || ext == JPG_EXTENSION || ext == JPEG_EXTENSION || ext == BMP_EXTENSION || ext == TGA_EXTENSION || ext == DDS_EXTENSION;
	}

	Asset* createAssetInstance(UID uid) const override
	{
		return new TextureAsset(uid);
	}

protected:
	bool loadExternal(const std::filesystem::path& path, DirectX::ScratchImage& out) override;
	void importTyped(const DirectX::ScratchImage& source, TextureAsset* destiny) override;
	uint64_t saveTyped(const TextureAsset* source, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, TextureAsset* dst) override;
};