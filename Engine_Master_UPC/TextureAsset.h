#pragma once
#include "Globals.h"
#include "Asset.h"

struct TextureImage
{
	uint32_t slicePitch = 0;
	uint32_t rowPitch = 0;
	std::vector<uint8_t> pixels;
};

class TextureAsset : public Asset
{
public:
	friend class TextureImporter;
	TextureAsset() {}
	TextureAsset(UID id) : Asset(id, AssetType::TEXTURE) {}

	uint32_t	getWidth() const { return width; }
	uint32_t	getHeight() const { return height; }
	uint32_t	getMipCount() const { return mipCount; }
	uint32_t	getArraySize() const { return arraySize; }
	DXGI_FORMAT getFormat() const { return format; }
	uint32_t	getImageCount() const { return imageCount; }
	std::vector<TextureImage>& getImages() const { return images; }

private:
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipCount = 0;
	uint32_t arraySize = 0;
	uint32_t imageCount = 0;

	mutable std::vector<TextureImage> images = {};

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};
