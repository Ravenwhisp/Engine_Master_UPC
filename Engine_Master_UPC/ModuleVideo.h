#pragma once

#include "Module.h"

#include <filesystem>
#include <memory>
#include <vector>

#include <xaudio2.h>

class VideoPlayback;

class ModuleVideo final : public Module
{
private:
	std::vector<std::unique_ptr<VideoPlayback>> m_activeVideos;

	IXAudio2* m_xAudio = nullptr;
	IXAudio2MasteringVoice* m_masterVoice = nullptr;

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
	VideoPlayback* getPlayingVideo() const;
	const std::vector<std::unique_ptr<VideoPlayback>>& getActiveVideos() const { return m_activeVideos; }
#pragma endregion
};