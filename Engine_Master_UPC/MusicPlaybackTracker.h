#pragma once

#include "PlayingSound.h"
#include <AK/SoundEngine/Common/AkCallback.h>

#include <mutex>
#include <vector>

class MusicPlaybackTracker
{
private:
	std::vector<PlayingSound> m_playingSounds;
	std::vector<PlayingSound> m_pendingPlayingSoundsToAdd;
	std::vector<uint32_t> m_pendingPlayingSoundsToRemove;

	std::mutex m_mutex;

public:
	void update();
	void cleanUp();

	const std::vector<PlayingSound>& getPlayingSounds() const;

	void queuePlayingSoundToAdd(const PlayingSound& playingSound);
	void queuePlayingSoundToRemove(uint32_t playingID);

	static AkCallbackFunc getCallbackFunction();

private:
	static void callback(AkCallbackType callbackType, AkEventCallbackInfo* eventCallbackInfo, void* callbackInfo, void* cookie);

	void removePlayingSound(uint32_t playingID);
};