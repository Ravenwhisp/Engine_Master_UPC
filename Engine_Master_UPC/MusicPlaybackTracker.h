#pragma once

#include "PlayingSound.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCallback.h>

#include <mutex>
#include <vector>

class MusicPlaybackTracker
{
public:
	void update();
	void cleanUp();

	const std::vector<PlayingSound>& getPlayingSounds() const;

	void queuePlayingSoundToAdd(const PlayingSound& playingSound);
	void queuePlayingSoundToRemove(AkPlayingID playingID);

	static AkCallbackFunc getCallbackFunction();

private:
	static void callback(AkCallbackType callbackType, AkEventCallbackInfo* eventCallbackInfo, void* callbackInfo, void* cookie);

	void removePlayingSound(AkPlayingID playingID);

private:
	std::vector<PlayingSound> m_playingSounds;
	std::vector<PlayingSound> m_pendingPlayingSoundsToAdd;
	std::vector<AkPlayingID> m_pendingPlayingSoundsToRemove;

	std::mutex m_mutex;
};