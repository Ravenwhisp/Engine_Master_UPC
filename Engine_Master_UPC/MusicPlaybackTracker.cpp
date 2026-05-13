#include "Globals.h"
#include "MusicPlaybackTracker.h"

void MusicPlaybackTracker::update()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (const PlayingSound& playingSound : m_pendingPlayingSoundsToAdd)
	{
		m_playingSounds.push_back(playingSound);
	}

	m_pendingPlayingSoundsToAdd.clear();

	for (AkPlayingID playingID : m_pendingPlayingSoundsToRemove)
	{
		removePlayingSound(playingID);
	}

	m_pendingPlayingSoundsToRemove.clear();
}

void MusicPlaybackTracker::cleanUp()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_pendingPlayingSoundsToAdd.clear();
	m_pendingPlayingSoundsToRemove.clear();
	m_playingSounds.clear();
}

const std::vector<PlayingSound>& MusicPlaybackTracker::getPlayingSounds() const
{
	return m_playingSounds;
}

void MusicPlaybackTracker::queuePlayingSoundToAdd(const PlayingSound& playingSound)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_pendingPlayingSoundsToAdd.push_back(playingSound);
}

void MusicPlaybackTracker::queuePlayingSoundToRemove(uint32_t playingID)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_pendingPlayingSoundsToRemove.push_back(playingID);
}

AkCallbackFunc MusicPlaybackTracker::getCallbackFunction()
{
	return reinterpret_cast<AkCallbackFunc>(&MusicPlaybackTracker::callback);
}

void MusicPlaybackTracker::callback(AkCallbackType callbackType, AkEventCallbackInfo* eventCallbackInfo, void* callbackInfo, void* cookie)
{
	if (callbackType != AK_EndOfEvent)
	{
		return;
	}

	if (eventCallbackInfo == nullptr || cookie == nullptr)
	{
		return;
	}

	MusicPlaybackTracker* tracker = static_cast<MusicPlaybackTracker*>(cookie);

	tracker->queuePlayingSoundToRemove(eventCallbackInfo->playingID);
}

void MusicPlaybackTracker::removePlayingSound(uint32_t playingID)
{
	for (auto it = m_playingSounds.begin(); it != m_playingSounds.end(); ++it)
	{
		if (it->playingID != playingID)
		{
			continue;
		}

		m_playingSounds.erase(it);
		return;
	}
}