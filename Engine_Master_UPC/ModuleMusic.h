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
	
	WwiseBank m_initBnk;
	std::vector<WwiseBank> m_banks;

public:
	ModuleMusic();
	~ModuleMusic();

#pragma region GameLoop
	bool init() override;
	void update() override;
	bool cleanUp() override;
#pragma endregion

#pragma region API
	void postGLobalEvent(const char* bankName, const char* eventName);
	void stopEvent(uint32_t playingID);
	void pauseEvent(uint32_t playingID);
	void resumeEvent(uint32_t playingID);
#pragma endregion

#pragma region Extra
	std::vector<WwiseBank>& getBankList() { return m_banks; }
	const std::vector<PlayingSound>& getPlayingSounds() const { return m_playbackTracker.getPlayingSounds(); }
	
	void unloadAllBanks();
	bool loadBank(const std::string& bankName);
	bool unloadBank(const std::string& bankName);
#pragma endregion

private:
	bool loadBanksFromFolder();
};