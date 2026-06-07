#include "pch.h"
#include "DeathSound.h"

namespace
{
    constexpr const char* k_bank = "BoundByDeath.bnk";

    constexpr const char* k_lightSwing      = "Play_Death_Light_Swing";
    constexpr const char* k_lightImpact     = "Play_Death_Light_Impact";
    constexpr const char* k_heavySwing      = "Play_Death_Heavy_Swing";
    constexpr const char* k_heavyImpact     = "Play_Death_Heavy_Impact";
    constexpr const char* k_chargeRelease   = "Play_Death_Charge_Release";
    constexpr const char* k_dashWhoosh      = "Play_Death_Dash_Whoosh";
    constexpr const char* k_dashImpact      = "Play_Death_Dash_Impact";
    constexpr const char* k_tauntShout      = "Play_Death_Taunt_Shout";
    constexpr const char* k_markApply       = "Play_Death_Mark_Apply";
    constexpr const char* k_hurt            = "Play_Death_Hurt";
    constexpr const char* k_down            = "Play_Death_Down";
    constexpr const char* k_lockTarget      = "Play_Death_Lock_Target";
    constexpr const char* k_switchTarget    = "Play_Death_Switch_Target";

    constexpr const char* k_chargeLoopStart = "Play_Death_Charge_Loop";
    constexpr const char* k_chargeLoopStop  = "Stop_Death_Charge_Loop";
    constexpr const char* k_hoverLoopStart  = "Play_Death_Hover_Loop";
    constexpr const char* k_hoverLoopStop   = "Stop_Death_Hover_Loop";

    // Tuned to match animation contact frame (swing windup → contact).
    constexpr float k_impactDelay       = 0.20f;
    // R2 must be held this long before the charge loop actually starts to sound.
    // A quicker release (heavy swing without charge) plays no charge sound at all.
    constexpr float k_chargeLoopMinHold = 0.20f;
}

DeathSound::DeathSound(GameObject* owner)
    : Script(owner)
{
}

void DeathSound::Start()
{
    m_source = AudioAPI::getSoundSourceComponent(getOwner());
    if (m_source == nullptr)
    {
        Debug::error("[DeathSound] No SOUND_SOURCE component on '%s'.",
                     GameObjectAPI::getName(getOwner()));
    }
}

void DeathSound::Update()
{
    const float dt = Time::getDeltaTime();

    for (size_t i = 0; i < m_pendingEvents.size(); )
    {
        m_pendingEvents[i].timeRemaining -= dt;
        if (m_pendingEvents[i].timeRemaining <= 0.0f)
        {
            postEvent(m_pendingEvents[i].eventName);
            m_pendingEvents[i] = m_pendingEvents.back();
            m_pendingEvents.pop_back();
        }
        else
        {
            ++i;
        }
    }

    if (m_chargeLoopArmTimer >= 0.0f)
    {
        m_chargeLoopArmTimer -= dt;
        if (m_chargeLoopArmTimer <= 0.0f)
        {
            m_chargeLoopArmTimer = -1.0f;
            m_chargeLoopID = postEvent(k_chargeLoopStart);
        }
    }
}

uint32_t DeathSound::postEvent(const char* eventName)
{
    if (m_source == nullptr)
    {
        return 0;
    }
    return AudioAPI::postEvent(m_source, k_bank, eventName);
}

void DeathSound::postEventDelayed(const char* eventName, float delay)
{
    if (delay <= 0.0f)
    {
        postEvent(eventName);
        return;
    }
    m_pendingEvents.push_back({ eventName, delay });
}

void DeathSound::playLightSwing()    { postEvent(k_lightSwing); }
void DeathSound::playLightImpact()   { postEventDelayed(k_lightImpact, k_impactDelay); }
void DeathSound::playHeavySwing()    { postEvent(k_heavySwing); }
void DeathSound::playHeavyImpact()   { postEventDelayed(k_heavyImpact, k_impactDelay); }
void DeathSound::playChargeRelease() { postEventDelayed(k_chargeRelease, k_impactDelay); }
void DeathSound::playDashWhoosh()    { postEvent(k_dashWhoosh); }
void DeathSound::playDashImpact()    { postEvent(k_dashImpact); }
void DeathSound::playTauntShout()    { postEvent(k_tauntShout); }
void DeathSound::playMarkApply()     { postEventDelayed(k_markApply, k_impactDelay); }
void DeathSound::playHurt()          { postEvent(k_hurt); }
void DeathSound::playDown()          { postEvent(k_down); }
void DeathSound::playLockTarget()    { postEvent(k_lockTarget); }
void DeathSound::playSwitchTarget()  { postEvent(k_switchTarget); }

void DeathSound::startChargeLoop()
{
    if (m_chargeLoopID != 0 || m_chargeLoopArmTimer >= 0.0f)
    {
        return;
    }
    m_chargeLoopArmTimer = k_chargeLoopMinHold;
}

void DeathSound::stopChargeLoop()
{
    if (m_chargeLoopArmTimer >= 0.0f)
    {
        // Released before the loop ever started — just disarm, no sound posted.
        m_chargeLoopArmTimer = -1.0f;
        return;
    }
    if (m_chargeLoopID == 0)
    {
        return;
    }
    postEvent(k_chargeLoopStop);
    m_chargeLoopID = 0;
}

void DeathSound::startHoverLoop()
{
    if (m_hoverLoopID != 0)
    {
        return;
    }
    m_hoverLoopID = postEvent(k_hoverLoopStart);
}

void DeathSound::stopHoverLoop()
{
    if (m_hoverLoopID == 0)
    {
        return;
    }
    postEvent(k_hoverLoopStop);
    m_hoverLoopID = 0;
}

void DeathSound::setHoverActive(bool active)
{
    if (active)
    {
        startHoverLoop();
    }
    else
    {
        stopHoverLoop();
    }
}

void DeathSound::stopAllLoops()
{
    stopChargeLoop();
    stopHoverLoop();
    m_pendingEvents.clear();
}

IMPLEMENT_SCRIPT(DeathSound)
