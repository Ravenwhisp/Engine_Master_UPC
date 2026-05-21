#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

class CAkDefaultIOHookDeferred;

class WwiseManager
{
private:
	static constexpr const wchar_t* WWISE_BASE_PATH = L"Assets\\Audio\\";

	static constexpr AkGameObjectID GLOBAL_GAME_OBJECT = 1;
	static constexpr AkGameObjectID DEFAULT_LISTENER_GAME_OBJECT = 2;

private:
	CAkDefaultIOHookDeferred* m_lowLevelIO = nullptr;

	bool m_memoryCreated = false;
	bool m_streamCreated = false;
	bool m_lowLevelIOCreated = false;
	bool m_soundEngineCreated = false;
	bool m_commCreated = false;

	bool m_globalGameObjectRegistered = false;
	bool m_defaultListenerGameObjectRegistered = false;

public:
	WwiseManager();
	~WwiseManager();

	bool init();
	void update();
	void cleanUp();

	bool isInitialized() const;

	AkGameObjectID getGlobalGameObject() const;
	AkGameObjectID getDefaultListenerGameObject() const;

	bool registerGameObject(AkGameObjectID gameObjectID, const char* name);
	void unregisterGameObject(AkGameObjectID gameObjectID);

	void setDefaultListener(AkGameObjectID listenerGameObjectID);
	void setListeners(AkGameObjectID emitterGameObjectID, const AkGameObjectID* listenerGameObjectIDs, AkUInt32 listenerCount);

private:
	bool initMemory();
	bool initStream();
	bool initLowLevelIO();
	bool initSoundEngine();
	bool initComm();
	bool registerDefaultGameObjects();
	void unregisterDefaultGameObjects();
};