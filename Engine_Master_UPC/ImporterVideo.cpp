#include "Globals.h"
#include "ImporterVideo.h"
#include "VideoAsset.h"

#include <fstream>

bool ImporterVideo::loadExternal(const std::filesystem::path& path, VideoSourceData& out)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);

	if (!file)
		return false;

	const std::streamsize size = file.tellg();

	if (size <= 0)
		return false;

	file.seekg(0, std::ios::beg);
	out.data.resize(static_cast<size_t>(size));

	if (!file.read(reinterpret_cast<char*>(out.data.data()), size))
		return false;

	return true;
}

void ImporterVideo::importTyped(const VideoSourceData& source, VideoAsset* dst)
{
	dst->setVideoData(source.data);
}
