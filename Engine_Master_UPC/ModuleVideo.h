#pragma once

#include "Module.h"

#include <filesystem>
#include <memory>
#include <vector>

class VideoPlayback;

class ModuleVideo final : public Module
{
private:
	std::vector<std::unique_ptr<VideoPlayback>> m_activeVideos;

public:
	ModuleVideo();
	~ModuleVideo() override;

#pragma region GameLoop
	bool init() override;
	void update() override;
	bool cleanUp() override;
#pragma endregion

#pragma region API
	VideoPlayback* playVideo(const std::filesystem::path& path);
	void stopVideo(VideoPlayback* video);
	void stopAllVideos();
#pragma endregion

#pragma region Extra
	const std::vector<std::unique_ptr<VideoPlayback>>& getActiveVideos() const { return m_activeVideos; }
#pragma endregion
};