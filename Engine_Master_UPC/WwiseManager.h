#pragma once

#include <AK/SoundEngine/Common/AkTypes.h>

#include "SimpleMath.h"
using DirectX::SimpleMath::Matrix;

#include <vector>

class CAkDefaultIOHookDeferred;

class WwiseManager
{
private:
	static constexpr const wchar_t* WWISE_BASE_PATH = L"Assets\\Audio\\";

	static constexpr AkGameObjectID GLOBAL_GAME_OBJECT = 1;
	static constexpr AkGameObjectID GLOBAL_LISTENER_GAME_OBJECT = 2;

private:
	CAkDefaultIOHookDeferred* m_lowLevelIO = nullptr;

	std::vector<AkGameObjectID> m_sceneListeners;

	bool m_memoryCreated = false;
	bool m_streamCreated = false;
	bool m_lowLevelIOCreated = false;
	bool m_soundEngineCreated = false;
	bool m_commCreated = false;

	bool m_globalGameObjectRegistered = false;
	bool m_globalListenerGameObjectRegistered = false;

public:
	WwiseManager();
	~WwiseManager();

	bool init();
	void update();
	void cleanUp();

	AkGameObjectID getGlobalGameObject() const;
	AkGameObjectID getGlobalListenerGameObject() const;

	bool registerGameObject(AkGameObjectID id, const char* name);
	void unregisterGameObject(AkGameObjectID id);

	bool registerListener(AkGameObjectID id, const char* name);
	void unregisterListener(AkGameObjectID id);

	void setGameObjectTransform(AkGameObjectID gameObjectID, const AkTransform& transform);

	const std::vector<AkGameObjectID>& getListeners() const;

	void setListeners(AkGameObjectID emitterID, const AkGameObjectID* listenerIDs, AkUInt32 listenerCount);

private:
	bool initMemory();
	bool initStream();
	bool initLowLevelIO();
	bool initSoundEngine();
	bool initComm();

	bool registerDefaultGameObjects();
	void unregisterDefaultGameObjects();
};