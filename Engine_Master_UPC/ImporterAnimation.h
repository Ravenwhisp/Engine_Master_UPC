#pragma once
#include "ImporterNative.h"
#include "AnimationAsset.h"

class ImporterAnimation : public ImporterNative<AnimationAsset, AssetType::ANIMATION>
{
public:
	bool canImport(const std::filesystem::path& path) const override
	{
		return path.extension().string() == ANIMATION_EXTENSION;
	}

	Asset* createAssetInstance(const MD5Hash& uid) const override;

protected:
	bool     importNative(const std::filesystem::path& path, AnimationAsset* dst) override;
	uint64_t saveTyped(const AnimationAsset* source, uint8_t** outBuffer)      override;
	void     loadTyped(const uint8_t* buffer, AnimationAsset* dst)       override;
};