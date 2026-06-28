#include "Globals.h"
#include "WwiseManager.h"

#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h>
#endif

#include <AkDefaultIOHookDeferred.h>

#include <algorithm>

WwiseManager::WwiseManager()
{
	m_lowLevelIO = new CAkDefaultIOHookDeferred();
}

WwiseManager::~WwiseManager()
{
	delete m_lowLevelIO;
	m_lowLevelIO = nullptr;
}

bool WwiseManager::init()
{
	if (!initMemory()) return false;
	if (!initStream()) return false;
	if (!initLowLevelIO()) return false;
	if (!initSoundEngine()) return false;

#ifndef AK_OPTIMIZED
	if (!initComm()) return false;
#endif

	if (!registerDefaultGameObjects()) return false;

	return true;
}

void WwiseManager::update()
{
	if (m_soundEngineCreated)
	{
		AK::SoundEngine::RenderAudio();
	}
}

void WwiseManager::stopAll()
{
	if (m_soundEngineCreated)
	{
		AK::SoundEngine::StopAll();
	}
}

void WwiseManager::cleanUp()
{
	unregisterDefaultGameObjects();

#ifndef AK_OPTIMIZED
	if (m_commCreated)
	{
		AK::Comm::Term();
		m_commCreated = false;
	}
#endif

	if (m_soundEngineCreated)
	{
		AK::SoundEngine::Term();
		m_soundEngineCreated = false;
	}

	if (m_lowLevelIOCreated)
	{
		m_lowLevelIO->Term();
		m_lowLevelIOCreated = false;
	}

	if (m_streamCreated && AK::IAkStreamMgr::Get())
	{
		AK::IAkStreamMgr::Get()->Destroy();
		m_streamCreated = false;
	}

	if (m_memoryCreated)
	{
		AK::MemoryMgr::Term();
		m_memoryCreated = false;
	}
}

AkGameObjectID WwiseManager::getGlobalGameObject() const
{
	return GLOBAL_GAME_OBJECT;
}

AkGameObjectID WwiseManager::getGlobalListenerGameObject() const
{
	return GLOBAL_LISTENER_GAME_OBJECT;
}

bool WwiseManager::registerGameObject(AkGameObjectID id, const char* name)
{
	if (!m_soundEngineCreated)
	{
		return false;
	}

	if (AK::SoundEngine::RegisterGameObj(id, name) != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed registering GameObject: %llu", id);
		return false;
	}

	return true;
}

void WwiseManager::unregisterGameObject(AkGameObjectID id)
{
	if (!m_soundEngineCreated)
	{
		return;
	}

	AK::SoundEngine::UnregisterGameObj(id);
}

bool WwiseManager::registerListener(AkGameObjectID id, const char* name)
{
	if (!registerGameObject(id, name))
	{
		return false;
	}

	if (std::find(m_sceneListeners.begin(), m_sceneListeners.end(), id) == m_sceneListeners.end())
	{
		m_sceneListeners.push_back(id);
	}

	return true;
}

void WwiseManager::unregisterListener(AkGameObjectID id)
{
	m_sceneListeners.erase(
		std::remove(m_sceneListeners.begin(), m_sceneListeners.end(), id),
		m_sceneListeners.end()
	);

	unregisterGameObject(id);
}

void WwiseManager::setGameObjectTransform(AkGameObjectID gameObjectID, const AkTransform& transform)
{
	if (!m_soundEngineCreated)
	{
		return;
	}

	AK::SoundEngine::SetPosition(gameObjectID, transform);
}

const std::vector<AkGameObjectID>& WwiseManager::getListeners() const
{
	return m_sceneListeners;
}

void WwiseManager::setListeners(AkGameObjectID emitterID, const AkGameObjectID* listenerIDs, AkUInt32 listenerCount)
{
	if (!m_soundEngineCreated)
	{
		return;
	}

	if (listenerIDs == nullptr || listenerCount == 0)
	{
		return;
	}

	AK::SoundEngine::SetListeners(emitterID, listenerIDs, listenerCount);
}

bool WwiseManager::setState(const char* stateGroup, const char* stateValue)
{
	if (!m_soundEngineCreated)
	{
		DEBUG_ERROR("[Wwise Manager] Cannot set State. SoundEngine is not initialized");
		return false;
	}

	const AKRESULT result = AK::SoundEngine::SetState(stateGroup, stateValue);

	if (result != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed setting State: %s -> %s", stateGroup, stateValue);
		return false;
	}

	DEBUG_LOG("[Wwise Manager] State set: %s -> %s", stateGroup, stateValue);
	return true;
}

bool WwiseManager::setSwitch(const char* switchGroup, const char* switchValue, AkGameObjectID emitterID)
{
	if (!m_soundEngineCreated)
	{
		DEBUG_ERROR("[Wwise Manager] Cannot set Switch. SoundEngine is not initialized");
		return false;
	}

	const AKRESULT result = AK::SoundEngine::SetSwitch(switchGroup, switchValue, emitterID);

	if (result != AK_Success)
	{
		DEBUG_ERROR(
			"[Wwise Manager] Failed setting Switch: %s -> %s | GameObject: %llu",
			switchGroup,
			switchValue,
			emitterID
		);

		return false;
	}

	DEBUG_LOG(
		"[Wwise Manager] Switch set: %s -> %s | GameObject: %llu",
		switchGroup,
		switchValue,
		emitterID
	);

	return true;
}

bool WwiseManager::setRTPC(const char* rtpcName, float value)
{
	if (!m_soundEngineCreated)
	{
		DEBUG_ERROR("[Wwise Manager] Cannot set RTPC. SoundEngine is not initialized");
		return false;
	}

	const AKRESULT result = AK::SoundEngine::SetRTPCValue(rtpcName, value);

	if (result != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed setting RTPC: %s = %.3f", rtpcName, value);
		return false;
	}

	DEBUG_LOG("[Wwise Manager] RTPC set: %s = %.3f", rtpcName, value);
	return true;
}

bool WwiseManager::initMemory()
{
	AkMemSettings memSettings;
	AK::MemoryMgr::GetDefaultSettings(memSettings);

	if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed initializing MemoryMgr");
		return false;
	}

	m_memoryCreated = true;
	return true;
}

bool WwiseManager::initStream()
{
	AkStreamMgrSettings streamSettings;
	AK::StreamMgr::GetDefaultSettings(streamSettings);

	if (!AK::StreamMgr::Create(streamSettings))
	{
		DEBUG_ERROR("[Wwise Manager] Failed creating StreamMgr");
		return false;
	}

	m_streamCreated = true;
	return true;
}

bool WwiseManager::initLowLevelIO()
{
	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

	if (m_lowLevelIO->Init(deviceSettings) != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed initializing LowLevelIO");
		return false;
	}

	m_lowLevelIOCreated = true;
	m_lowLevelIO->SetBasePath(WWISE_BASE_PATH);

	return true;
}

bool WwiseManager::initSoundEngine()
{
	AkInitSettings initSettings;
	AkPlatformInitSettings platformSettings;

	AK::SoundEngine::GetDefaultInitSettings(initSettings);
	AK::SoundEngine::GetDefaultPlatformInitSettings(platformSettings);

	if (AK::SoundEngine::Init(&initSettings, &platformSettings) != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed initializing SoundEngine");
		return false;
	}

	m_soundEngineCreated = true;
	return true;
}

#ifndef AK_OPTIMIZED
bool WwiseManager::initComm()
{
	AkCommSettings commSettings;
	AK::Comm::GetDefaultInitSettings(commSettings);

	if (AK::Comm::Init(commSettings) == AK_Success)
	{
		m_commCreated = true;
	}

	return true;
}
#endif

bool WwiseManager::registerDefaultGameObjects()
{
	if (!registerGameObject(GLOBAL_GAME_OBJECT, "Global Audio"))
	{
		return false;
	}

	m_globalGameObjectRegistered = true;

	if (!registerGameObject(GLOBAL_LISTENER_GAME_OBJECT, "Global Listener"))
	{
		return false;
	}

	m_globalListenerGameObjectRegistered = true;

	setListeners(GLOBAL_GAME_OBJECT, &GLOBAL_LISTENER_GAME_OBJECT, 1);

	return true;
}

void WwiseManager::unregisterDefaultGameObjects()
{
	m_sceneListeners.clear();

	if (m_globalListenerGameObjectRegistered)
	{
		unregisterGameObject(GLOBAL_LISTENER_GAME_OBJECT);
		m_globalListenerGameObjectRegistered = false;
	}

	if (m_globalGameObjectRegistered)
	{
		unregisterGameObject(GLOBAL_GAME_OBJECT);
		m_globalGameObjectRegistered = false;
	}
}