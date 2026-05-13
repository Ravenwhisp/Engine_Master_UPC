#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

class WwiseManager
{
public:
	bool init();
	void update();
	void cleanUp();

	bool isInitialized() const;

	AkGameObjectID getMusicGameObject() const;
	AkGameObjectID getListenerGameObject() const;

private:
	bool initMemory();
	bool initStream();
	bool initLowLevelIO();
	bool initSoundEngine();
	bool initComm();
	bool registerGameObjects();

private:
	bool m_memoryCreated = false;
	bool m_streamCreated = false;
	bool m_lowLevelIOCreated = false;
	bool m_soundEngineCreated = false;
	bool m_commCreated = false;
	bool m_musicGameObjectRegistered = false;
	bool m_listenerGameObjectRegistered = false;
};