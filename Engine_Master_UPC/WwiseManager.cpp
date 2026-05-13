#include "Globals.h"
#include "WwiseManager.h"

#include <AK/SoundEngine/Common/AkMemoryMgrModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Comm/AkCommunication.h>

#include <AkDefaultIOHookDeferred.h>

namespace
{
	constexpr const wchar_t* WWISE_BASE_PATH = L"Assets\\Audio\\";

	constexpr AkGameObjectID MUSIC_GAME_OBJECT = 1;
	constexpr AkGameObjectID LISTENER_GAME_OBJECT = 2;

	CAkDefaultIOHookDeferred g_lowLevelIO;
}

bool WwiseManager::init()
{
	if (!initMemory()) return false;
	if (!initStream()) return false;
	if (!initLowLevelIO()) return false;
	if (!initSoundEngine()) return false;
	if (!initComm()) return false;
	if (!registerGameObjects()) return false;

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
	if (m_listenerGameObjectRegistered)
	{
		AK::SoundEngine::UnregisterGameObj(LISTENER_GAME_OBJECT);
		m_listenerGameObjectRegistered = false;
	}

	if (m_musicGameObjectRegistered)
	{
		AK::SoundEngine::UnregisterGameObj(MUSIC_GAME_OBJECT);
		m_musicGameObjectRegistered = false;
	}

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
		g_lowLevelIO.Term();
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

AkGameObjectID WwiseManager::getMusicGameObject() const
{
	return MUSIC_GAME_OBJECT;
}

AkGameObjectID WwiseManager::getListenerGameObject() const
{
	return LISTENER_GAME_OBJECT;
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

	if (g_lowLevelIO.Init(deviceSettings) != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed initializing LowLevelIO");
		return false;
	}

	m_lowLevelIOCreated = true;
	g_lowLevelIO.SetBasePath(WWISE_BASE_PATH);

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

bool WwiseManager::registerGameObjects()
{
	if (AK::SoundEngine::RegisterGameObj(MUSIC_GAME_OBJECT, "Music") != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed registering Music GameObject");
		return false;
	}

	m_musicGameObjectRegistered = true;

	if (AK::SoundEngine::RegisterGameObj(LISTENER_GAME_OBJECT, "Listener") != AK_Success)
	{
		DEBUG_ERROR("[Wwise Manager] Failed registering Listener GameObject");
		return false;
	}

	m_listenerGameObjectRegistered = true;

	AK::SoundEngine::SetDefaultListeners(&LISTENER_GAME_OBJECT, 1);
	AK::SoundEngine::SetListeners(MUSIC_GAME_OBJECT, &LISTENER_GAME_OBJECT, 1);

	return true;
}