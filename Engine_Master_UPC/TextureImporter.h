#pragma once
#include "TypedImporter.h"
#include <DirectXTex.h>

#include "TextureAsset.h"

class TextureImporter : public TypedImporter<DirectX::ScratchImage, TextureAsset>
{
public:

    bool canImport(const std::filesystem::path& path) const override
	{
		auto ext = path.extension().string();
		return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".dds";
	}

	Asset* createAssetInstance(int uid) const override
	{
		return new TextureAsset(uid);
	}

protected:
	bool loadExternal(const std::filesystem::path& path, DirectX::ScratchImage& out) override;
	void importTyped(const DirectX::ScratchImage& source, TextureAsset* destiny) override;
	uint64_t saveTyped(const TextureAsset* source, uint8_t** buffer) override;
	void loadTyped(const uint8_t* buffer, TextureAsset* dst) override;
};