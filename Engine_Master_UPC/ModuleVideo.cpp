#include "Globals.h"
#include "ModuleVideo.h"

#include "VideoPlayback.h"

#include <algorithm>

ModuleVideo::ModuleVideo() = default;
ModuleVideo::~ModuleVideo() = default;

#pragma region GameLoop
bool ModuleVideo::init()
{
	HRESULT result = XAudio2Create(&m_xAudio);

	if (FAILED(result))
	{
		DEBUG_ERROR("[Module Video] Could not initialize XAudio2");
		return false;
	}

	result = m_xAudio->CreateMasteringVoice(&m_masterVoice);

	if (FAILED(result))
	{
		DEBUG_ERROR("[Module Video] Could not create XAudio2 mastering voice");

		m_xAudio->Release();
		m_xAudio = nullptr;

		return false;
	}

	DEBUG_LOG("[Module Video] Initialized");

	return true;
}

void ModuleVideo::update()
{
	for (auto& video : m_activeVideos)
		video->update();
}

bool ModuleVideo::cleanUp()
{
	stopAllVideos();

	if (m_masterVoice)
	{
		m_masterVoice->DestroyVoice();
		m_masterVoice = nullptr;
	}

	if (m_xAudio)
	{
		m_xAudio->Release();
		m_xAudio = nullptr;
	}

	return true;
}
#pragma endregion

#pragma region API
VideoPlayback* ModuleVideo::playVideo(const std::vector<uint8_t>& data)
{
	if (!m_xAudio)
		return nullptr;

	auto video = std::make_unique<VideoPlayback>(m_xAudio);

	if (!video->loadFromBuffer(data.data(), data.size()))
		return nullptr;

	if (!video->play())
		return nullptr;

	VideoPlayback* result = video.get();
	m_activeVideos.push_back(std::move(video));

	return result;
}

VideoPlayback* ModuleVideo::playVideo(const std::filesystem::path& path)
{
	if (!m_xAudio)
		return nullptr;

	auto video = std::make_unique<VideoPlayback>(m_xAudio);

	if (!video->load(path))
		return nullptr;

	if (!video->play())
		return nullptr;

	VideoPlayback* result = video.get();
	m_activeVideos.push_back(std::move(video));

	return result;
}

void ModuleVideo::stopVideo(VideoPlayback* video)
{
	if (!video)
		return;

	const auto iterator = std::find_if(m_activeVideos.begin(), m_activeVideos.end(), [video](const std::unique_ptr<VideoPlayback>& current)
		{
			return current.get() == video;
		});

	if (iterator == m_activeVideos.end())
		return;

	(*iterator)->stop();
	m_activeVideos.erase(iterator);
}

void ModuleVideo::stopAllVideos()
{
	for (auto& video : m_activeVideos)
		video->stop();

	m_activeVideos.clear();
}
#pragma endregion

#pragma region Extra
VideoPlayback* ModuleVideo::getPlayingVideo() const
{
	for (const auto& video : m_activeVideos)
	{
		if (video && video->isPlaying())
			return video.get();
	}

	return nullptr;
}
#pragma endregion