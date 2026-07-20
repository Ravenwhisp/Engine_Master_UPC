#pragma once
#include "ImporterSource.h"
#include <DirectXTex.h>

#include "TextureAsset.h"


class ImporterTexture : public ImporterSource<DirectX::ScratchImage, TextureAsset, AssetType::TEXTURE>
{
public:

    bool canImport(const std::filesystem::path& path) const override
	{
		auto ext = path.extension().string();
		return ext == PNG_EXTENSION || ext == JPG_EXTENSION || ext == JPEG_EXTENSION || ext == BMP_EXTENSION || ext == TGA_EXTENSION || ext == DDS_EXTENSION || ext == HDR_EXTENSION;
	}

	Asset* createAssetInstance(AssetId& uid) const override
	{
		return new TextureAsset(uid);
	}

	bool import(const std::filesystem::path& path, Asset* outAsset) override;

protected:
	bool loadExternal(const std::filesystem::path& path, DirectX::ScratchImage& out) override;
	void importTyped(const DirectX::ScratchImage& source, TextureAsset* destiny) override;
};