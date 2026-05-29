#include "pch.h"
#include "LyrielSound.h"

namespace
{
    constexpr const char* k_bank = "BoundByDeath.bnk";

    constexpr const char* k_bowRelease       = "Play_Lyriel_Bow_Release";
    constexpr const char* k_arrowImpact      = "Play_Lyriel_Arrow_Impact";
    constexpr const char* k_chargedTense     = "Play_Lyriel_Charged_Tense";
    constexpr const char* k_chargedRelease   = "Play_Lyriel_Charged_Release";
    constexpr const char* k_chargedImpact    = "Play_Lyriel_Charged_Impact";
    constexpr const char* k_dashWhoosh       = "Play_Lyriel_Dash_Whoosh";
    constexpr const char* k_volleyRelease    = "Play_Lyriel_Volley_Release";
    constexpr const char* k_markExploit      = "Play_Lyriel_Mark_Exploit";
    constexpr const char* k_footsteps        = "Play_Lyriel_Footsteps";
    constexpr const char* k_hurt             = "Play_Lyriel_Hurt";
    constexpr const char* k_down             = "Play_Lyriel_Down";
    constexpr const char* k_lockTarget       = "Play_Lyriel_Lock_Target";
    constexpr const char* k_switchTarget     = "Play_Lyriel_Switch_Target";

    // R2 must be held this long before the tense loop becomes audible. A quicker
    // release plays no tense at all (just the release one-shot).
    constexpr float k_tenseLoopMinHold = 0.20f;

    // Approximate walking cadence. No animation hook is available yet, so we
    // tick footstep events on a timer while the player is moving.
    constexpr float k_footstepInterval = 0.45f;
}

LyrielSound::LyrielSound(GameObject* owner)
    : Script(owner)
{
}

void LyrielSound::Start()
{
    m_source = AudioAPI::getSoundSourceComponent(getOwner());
    if (m_source == nullptr)
    {
        Debug::error("[LyrielSound] No SOUND_SOURCE component on '%s'.",
                     GameObjectAPI::getName(getOwner()));
    }
}

void LyrielSound::Update()
{
    const float dt = Time::getDeltaTime();

    if (m_chargedTenseArmTimer >= 0.0f)
    {
        m_chargedTenseArmTimer -= dt;
        if (m_chargedTenseArmTimer <= 0.0f)
        {
            m_chargedTenseArmTimer = -1.0f;
            m_chargedTenseLoopID   = postEvent(k_chargedTense);
        }
    }

    if (m_footstepsActive)
    {
        m_footstepTimer -= dt;
        if (m_footstepTimer <= 0.0f)
        {
            const uint32_t playingID = postEvent(k_footsteps);
            Debug::log("[LyrielSound] Footstep posted (playingID=%u)", playingID);
            m_footstepTimer = k_footstepInterval;
        }
    }
}

uint32_t LyrielSound::postEvent(const char* eventName)
{
    if (m_source == nullptr)
    {
        return 0;
    }
    return AudioAPI::postEvent(m_source, k_bank, eventName);
}

void LyrielSound::playBowRelease()      { postEvent(k_bowRelease); }
void LyrielSound::playArrowImpact()     { postEvent(k_arrowImpact); }
void LyrielSound::playChargedRelease()  { postEvent(k_chargedRelease); }
void LyrielSound::playChargedImpact()   { postEvent(k_chargedImpact); }
void LyrielSound::playDashWhoosh()      { postEvent(k_dashWhoosh); }
void LyrielSound::playVolleyRelease()   { postEvent(k_volleyRelease); }
void LyrielSound::playMarkExploit()     { postEvent(k_markExploit); }
void LyrielSound::playHurt()            { postEvent(k_hurt); }
void LyrielSound::playDown()            { postEvent(k_down); }
void LyrielSound::playLockTarget()      { postEvent(k_lockTarget); }
void LyrielSound::playSwitchTarget()    { postEvent(k_switchTarget); }

void LyrielSound::startChargedTenseLoop()
{
    if (m_chargedTenseLoopID != 0 || m_chargedTenseArmTimer >= 0.0f)
    {
        return;
    }
    m_chargedTenseArmTimer = k_tenseLoopMinHold;
}

void LyrielSound::stopChargedTenseLoop()
{
    if (m_chargedTenseArmTimer >= 0.0f)
    {
        m_chargedTenseArmTimer = -1.0f;
        return;
    }
    if (m_chargedTenseLoopID == 0 || m_source == nullptr)
    {
        return;
    }
    AudioAPI::stopEvent(m_source, m_chargedTenseLoopID);
    m_chargedTenseLoopID = 0;
}

void LyrielSound::setFootstepsActive(bool active)
{
    if (m_footstepsActive == active)
    {
        return;
    }
    m_footstepsActive = active;
    // Reset timer so the first step after starting to move plays almost immediately.
    m_footstepTimer = active ? 0.0f : 0.0f;
}

void LyrielSound::stopAllLoops()
{
    stopChargedTenseLoop();
    m_footstepsActive = false;
    m_footstepTimer   = 0.0f;
}

IMPLEMENT_SCRIPT(LyrielSound)
