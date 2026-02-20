#pragma once
#include "TypedImporter.h"
#include <DirectXTex.h>
#include "Asset.h"


struct TextureImage
{
	uint32_t slicePitch = 0;
	uint32_t rowPitch = 0;
	std::vector<uint8_t> pixels;
};

class TextureAsset: public Asset
{
public:
	friend class TextureImporter;
	TextureAsset(int id) : Asset(id) {}

	uint32_t getWidth() const { return width; }
	uint32_t getHeight() const { return height; }
	uint32_t getMipCount() const { return mipCount; }
	uint32_t getArraySize() const { return arraySize; }
	DXGI_FORMAT getFormat() const { return format; }
	uint32_t getImageCount() const { return imageCount; }
	std::vector<TextureImage>& getImages() { return images; }
private:
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipCount = 0;
	uint32_t arraySize = 0;
	uint32_t imageCount = 0;

	std::vector<TextureImage> images = {};

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};

class TextureImporter : public TypedImporter<DirectX::ScratchImage, TextureAsset>
{
public:

    bool canImport(const std::filesystem::path& path) const override
	{
		auto ext = path.extension().string();
		return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga";
	}

	Asset* createAssetInstance() const override
	{
		return new TextureAsset(rand());
	}
protected:
	bool loadExternal(const std::filesystem::path& path, DirectX::ScratchImage& out) override;
	void importTyped(const DirectX::ScratchImage& source, TextureAsset* destiny) override;
	uint64_t saveTyped(const TextureAsset* source, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, TextureAsset* dst) override;
};