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

	// "Música ya arrancada" en esta sesión de play. Se resetea en stopAllSounds (Stop).
	bool m_musicStarted = false;

public:
	ModuleMusic();
	~ModuleMusic();

#pragma region GameLoop
	bool init() override;
	void update() override;
	bool cleanUp() override;
#pragma endregion

#pragma region API
	uint32_t postGlobalEvent(const char* bankName, const char* eventName);
	uint32_t postEvent(const char* bankName, const char* eventName, uint64_t emitterID);
	void stopEvent(uint32_t playingID);
	void pauseEvent(uint32_t playingID);
	void resumeEvent(uint32_t playingID);

	bool registerAudioGameObject(uint64_t gameObjectID, const char* name);
	void unregisterAudioGameObject(uint64_t gameObjectID);

	bool registerListener(uint64_t id, const char* name);
	void unregisterListener(uint64_t id);

	void setAudioGameObjectTransform(uint64_t gameObjectID, const Vector3& position, const Vector3& forward, const Vector3& up);

	void setState(const char* stateGroup, const char* stateValue);
	void setSwitch(const char* switchGroup, const char* switchValue, uint64_t emitterID);
	void setRTPC(const char* rtpcName, float value);

	// Para toda la música/SFX en curso. Se llama al detener la simulación (Stop).
	void stopAllSounds();

	// Flag de "música arrancada" para esta sesión. El MusicManager lo usa para postear
	// Play_Music una sola vez; entre escenas solo cambia el State (crossfade) y tras un
	// Stop (stopAllSounds lo resetea) vuelve a arrancar.
	bool isMusicStarted() const { return m_musicStarted; }
	void setMusicStarted(bool started) { m_musicStarted = started; }
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