#include "Globals.h"
#include "WwiseManager.h"

#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Comm/AkCommunication.h>

#include <AkDefaultIOHookDeferred.h>

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
	if (!initComm()) return false;
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

void WwiseManager::cleanUp()
{
	unregisterDefaultGameObjects();

	if (m_commCreated)
	{
		AK::Comm::Term();
		m_commCreated = false;
	}

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

bool WwiseManager::isInitialized() const
{
	return m_soundEngineCreated;
}

AkGameObjectID WwiseManager::getGlobalGameObject() const
{
	return GLOBAL_GAME_OBJECT;
}

AkGameObjectID WwiseManager::getDefaultListenerGameObject() const
{
	return DEFAULT_LISTENER_GAME_OBJECT;
}

bool WwiseManager::registerGameObject(AkGameObjectID gameObjectID, const char* name)
{
	if (!m_soundEngineCreated)
	{
		return false;
	}

	if (AK::SoundEngine::RegisterGameObj(gameObjectID, name) != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed registering GameObject: %llu", gameObjectID);
		return false;
	}

	return true;
}

void WwiseManager::unregisterGameObject(AkGameObjectID gameObjectID)
{
	if (!m_soundEngineCreated)
	{
		return;
	}

	AK::SoundEngine::UnregisterGameObj(gameObjectID);
}

void WwiseManager::setDefaultListener(AkGameObjectID listenerGameObjectID)
{
	if (!m_soundEngineCreated)
	{
		return;
	}

	AK::SoundEngine::SetDefaultListeners(&listenerGameObjectID, 1);
}

void WwiseManager::setListeners(AkGameObjectID emitterGameObjectID, const AkGameObjectID* listenerGameObjectIDs, AkUInt32 listenerCount)
{
	if (!m_soundEngineCreated)
	{
		return;
	}

	AK::SoundEngine::SetListeners(emitterGameObjectID, listenerGameObjectIDs, listenerCount);
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

bool WwiseManager::registerDefaultGameObjects()
{
	if (!registerGameObject(GLOBAL_GAME_OBJECT, "Global Audio"))
	{
		return false;
	}

	m_globalGameObjectRegistered = true;

	if (!registerGameObject(DEFAULT_LISTENER_GAME_OBJECT, "Default Listener"))
	{
		return false;
	}

	m_defaultListenerGameObjectRegistered = true;

	setDefaultListener(DEFAULT_LISTENER_GAME_OBJECT);
	setListeners(GLOBAL_GAME_OBJECT, &DEFAULT_LISTENER_GAME_OBJECT, 1);

	return true;
}

void WwiseManager::unregisterDefaultGameObjects()
{
	if (m_defaultListenerGameObjectRegistered)
	{
		unregisterGameObject(DEFAULT_LISTENER_GAME_OBJECT);
		m_defaultListenerGameObjectRegistered = false;
	}

	if (m_globalGameObjectRegistered)
	{
		unregisterGameObject(GLOBAL_GAME_OBJECT);
		m_globalGameObjectRegistered = false;
	}
}