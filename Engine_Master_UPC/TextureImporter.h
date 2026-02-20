#pragma once
#include "TypedImporter.h"
#include <DirectXTex.h>
#include "Asset.h"


class TextureAsset: public Asset
{
public:
	friend class TextureImporter;
	TextureAsset(int id) : Asset(id) {}
private:
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipCount = 0;
	UINT32 arraySize = 0;

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
protected:
	bool loadExternal(const std::filesystem::path& path, DirectX::ScratchImage& out) override;
	void importTyped(const DirectX::ScratchImage& source, TextureAsset* destiny) override;
	uint64_t saveTyped(const TextureAsset* source, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, TextureAsset* dst) override;
};