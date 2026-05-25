#include "pch.h"
#include "MusicSystemTester.h"

IMPLEMENT_SCRIPT_FIELDS(MusicSystemTester,
    SERIALIZED_STRING(m_bankName, "Bank Name"),
    SERIALIZED_STRING(m_eventName, "Event Name"),
    SERIALIZED_FLOAT(m_pauseAfterSeconds, "Pause After Seconds", 0.0f, 60.0f, 0.1f),
    SERIALIZED_FLOAT(m_resumeAfterSeconds, "Resume After Seconds", 0.0f, 60.0f, 0.1f),
    SERIALIZED_FLOAT(m_stopAfterSeconds, "Stop After Seconds", 0.0f, 60.0f, 0.1f),
    SERIALIZED_BOOL(m_playOnStart, "Play On Start"),
)

MusicSystemTester::MusicSystemTester(GameObject* owner)
    : Script(owner)
{
}

void MusicSystemTester::Start()
{
    m_soundSource = AudioAPI::getSoundSourceComponent(getOwner());

    if (m_soundSource == nullptr)
    {
        Debug::warn("[MusicSystemTester] GameObject '%s' has no ComponentSoundSource.", GameObjectAPI::getName(getOwner()));

        return;
    }

    m_timer = 0.0f;
    m_playingID = 0;
    m_currentStep = TestStep::None;

    if (m_playOnStart)
    {
        playMusicEvent();
    }
}

void MusicSystemTester::Update()
{
    if (m_soundSource == nullptr || m_playingID == 0)
    {
        return;
    }

    m_timer += Time::getDeltaTime();

    if (m_currentStep == TestStep::Playing && m_timer >= m_pauseAfterSeconds)
    {
        pauseMusicEvent();
        return;
    }

    if (m_currentStep == TestStep::Paused && m_timer >= m_resumeAfterSeconds)
    {
        resumeMusicEvent();
        return;
    }

    if (m_currentStep == TestStep::Resumed && m_timer >= m_stopAfterSeconds)
    {
        stopMusicEvent();
        return;
    }
}

void MusicSystemTester::playMusicEvent()
{
    if (m_soundSource == nullptr)
    {
        return;
    }

    m_playingID = AudioAPI::postEvent(m_soundSource, m_bankName.c_str(), m_eventName.c_str());

    if (m_playingID == 0)
    {
        Debug::warn("[MusicSystemTester] Failed to post event '%s' from bank '%s'.", m_eventName.c_str(), m_bankName.c_str());

        return;
    }

    m_currentStep = TestStep::Playing;

    Debug::log("[MusicSystemTester] Posted event '%s' from bank '%s'. PlayingID: %u", m_eventName.c_str(), m_bankName.c_str(), m_playingID
    );
}

void MusicSystemTester::pauseMusicEvent()
{
    AudioAPI::pauseEvent(m_soundSource, m_playingID);

    m_currentStep = TestStep::Paused;

    Debug::log("[MusicSystemTester] Paused event. PlayingID: %u", m_playingID
    );
}

void MusicSystemTester::resumeMusicEvent()
{
    AudioAPI::resumeEvent(m_soundSource, m_playingID);

    m_currentStep = TestStep::Resumed;

    Debug::log("[MusicSystemTester] Resumed event. PlayingID: %u", m_playingID
    );
}

void MusicSystemTester::stopMusicEvent()
{
    AudioAPI::stopEvent(m_soundSource, m_playingID);

    Debug::log("[MusicSystemTester] Stopped event. PlayingID: %u", m_playingID);

    m_playingID = 0;
    m_currentStep = TestStep::Stopped;
}

IMPLEMENT_SCRIPT(MusicSystemTester)