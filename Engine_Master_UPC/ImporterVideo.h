#pragma once
#include "ImporterSource.h"
#include "VideoAsset.h"
#include "Extensions.h"

#include <vector>
#include <string>
#include <cstdint>

struct VideoSourceData
{
	std::vector<uint8_t> data;
};

class ImporterVideo : public ImporterSource<VideoSourceData, VideoAsset, AssetType::VIDEO>
{
public:
	bool canImport(const std::filesystem::path& path) const override
	{
		const std::string ext = path.extension().string();
		return ext == VIDEO_EXTENSION
			|| ext == ".avi" || ext == ".mkv" || ext == ".webm" || ext == ".mov"
			|| ext == ".mpg" || ext == ".mpeg" || ext == ".wmv" || ext == ".flv";
	}

	Asset* createAssetInstance(AssetId& uid) const override
	{
		return new VideoAsset(uid);
	}

protected:
	bool loadExternal(const std::filesystem::path& path, VideoSourceData& out) override;
	void importTyped(const VideoSourceData& source, VideoAsset* dst) override;
};
