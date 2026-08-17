#include "Globals.h"
#include "ModuleVideo.h"

#include "VideoPlayback.h"

#include <algorithm>

ModuleVideo::ModuleVideo() = default;
ModuleVideo::~ModuleVideo() = default;

#pragma region GameLoop
bool ModuleVideo::init()
{
	return true;
}

void ModuleVideo::update()
{
	for (auto& video : m_activeVideos)
	{
		video->update();
	}
}

bool ModuleVideo::cleanUp()
{
	stopAllVideos();

	return true;
}
#pragma endregion

#pragma region API
VideoPlayback* ModuleVideo::playVideo(const std::filesystem::path& path)
{
	auto video = std::make_unique<VideoPlayback>();

	if (!video->load(path))
	{
		return nullptr;
	}

	if (!video->play())
	{
		return nullptr;
	}

	VideoPlayback* result = video.get();

	m_activeVideos.push_back(std::move(video));

	return result;
}

void ModuleVideo::stopVideo(VideoPlayback* video)
{
	if (!video)
	{
		return;
	}

	const auto iterator = std::find_if(m_activeVideos.begin(), m_activeVideos.end(), [video](const std::unique_ptr<VideoPlayback>& current)
		{
			return current.get() == video;
		});

	if (iterator == m_activeVideos.end())
	{
		return;
	}

	(*iterator)->stop();
	m_activeVideos.erase(iterator);
}

void ModuleVideo::stopAllVideos()
{
	for (auto& video : m_activeVideos)
	{
		video->stop();
	}

	m_activeVideos.clear();
}
#pragma endregion