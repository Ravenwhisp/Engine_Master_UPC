#pragma once
#include "Globals.h"
#include "Asset.h"
#include "TextureImage.h"


class TextureAsset : public Asset
{
private:
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipCount = 0;
	uint32_t arraySize = 0;
	uint32_t imageCount = 0;

	mutable std::vector<TextureImage> images = {};

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

public:
	friend class ImporterTexture;

	TextureAsset() {}
	TextureAsset(UID id) : Asset(id, AssetType::TEXTURE) {}

	uint32_t	getWidth() const { return width; }
	uint32_t	getHeight() const { return height; }
	uint32_t	getMipCount() const { return mipCount; }
	uint32_t	getArraySize() const { return arraySize; }
	DXGI_FORMAT getFormat() const { return format; }
	uint32_t	getImageCount() const { return imageCount; }
	std::vector<TextureImage>& getImages() const { return images; }

#pragma region Serialization
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(
			cereal::base_class<Asset>(this),
			width,
			height,
			mipCount,
			arraySize,
			imageCount,
			images,
			format_as_uint()
		);
	}

	uint32_t& format_as_uint()
	{
		return reinterpret_cast<uint32_t&>(format);
	}
#pragma endregion
};

CEREAL_REGISTER_TYPE(TextureAsset)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Asset, TextureAsset)
