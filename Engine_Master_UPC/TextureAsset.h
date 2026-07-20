#pragma once
#include "Globals.h"
#include "Asset.h"
#include "IArchive.h"

#include "TextureImage.h"

class TextureAsset : public Asset
{
public:
	friend class ImporterTexture;

	TextureAsset() {}
	TextureAsset(AssetId& id) : Asset(id, AssetType::TEXTURE) {}

	uint32_t	getWidth() const { return width; }
	uint32_t	getHeight() const { return height; }
	uint32_t	getMipCount() const { return mipCount; }
	uint32_t	getArraySize() const { return arraySize; }
	DXGI_FORMAT getFormat() const { return format; }
	uint32_t	getImageCount() const { return imageCount; }
	std::vector<TextureImage>& getImages() const { return images; }

	void drawUI() override;

	std::unique_ptr<ImportSettings> createDefaultImportSettings() const override;

	void serialize(IArchive& archive) override
	{
		archive.serialize(width);
		archive.serialize(height);
		archive.serialize(mipCount);
		archive.serialize(arraySize);
		archive.serialize(imageCount);

		uint32_t formatUint = static_cast<uint32_t>(format);
		archive.serialize(formatUint);
		format = static_cast<DXGI_FORMAT>(formatUint);

		if (archive.mode() == ArchiveMode::Input)
			images.resize(imageCount);

		for (uint32_t i = 0; i < imageCount; ++i)
		{
			auto& img = images[i];
			archive.serialize(img.rowPitch);
			archive.serialize(img.slicePitch);
			uint32_t dataSize = static_cast<uint32_t>(img.pixels.size());
			archive.serialize(dataSize);
			if (archive.mode() == ArchiveMode::Input)
				img.pixels.resize(dataSize);
			archive.serializeRaw(img.pixels.data(), dataSize);
		}
	}

private:
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipCount = 0;
	uint32_t arraySize = 0;
	uint32_t imageCount = 0;

	mutable std::vector<TextureImage> images = {};

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};
