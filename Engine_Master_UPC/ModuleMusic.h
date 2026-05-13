#pragma once

#include "Module.h"
#include "WwiseManager.h"
#include "WwiseBank.h"
#include "PlayingSound.h"
#include "MusicPlaybackTracker.h"

#include <vector>

class ModuleMusic : public Module
{
private:
	WwiseManager m_wwiseManager;
	MusicPlaybackTracker m_playbackTracker;

	std::vector<WwiseBank> m_banks;

public:
	ModuleMusic();
	~ModuleMusic();

	bool init() override;
	void update() override;
	bool cleanUp() override;

	const std::vector<WwiseBank>& getBankList() const { return m_banks; }
	const std::vector<PlayingSound>& getPlayingSounds() const { return m_playbackTracker.getPlayingSounds(); }

	void postEvent(const char* bankName, const char* eventName);
	void stopEvent(uint32_t playingID);

private:
	bool loadBanksFromFolder();
};