#include "pch.h"
#include "EnemySound.h"

namespace
{
    // Every Paladin/Archer event lives in the Level1 bank (lazy-loaded on level entry).
    constexpr const char* k_bank = "Level1.bnk";

    // Minimum gap between hurt one-shots so continuous/overlapping damage can't
    // machine-gun the grunt.
    constexpr float k_hurtRetriggerCooldown = 0.25f;

    // Footstep cadence and the watchdog window after the last step before they stop.
    constexpr float k_footstepInterval = 0.40f;
    constexpr float k_movingWatchdog   = 0.15f;
}

EnemySound::EnemySound(GameObject* owner)
    : Script(owner)
{
}

void EnemySound::Start()
{
    m_source = AudioAPI::getSoundSourceComponent(getOwner());
    if (m_source == nullptr)
    {
        Debug::error("[EnemySound] No SOUND_SOURCE component on '%s'.",
                     GameObjectAPI::getName(getOwner()));
    }
}

void EnemySound::Update()
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

    if (m_hurtCooldownTimer > 0.0f)
    {
        m_hurtCooldownTimer -= dt;
    }

    if (m_movingTimer > 0.0f)
    {
        m_movingTimer -= dt;

        m_footstepTimer -= dt;
        if (m_footstepTimer <= 0.0f)
        {
            postEvent(evFootstep());
            m_footstepTimer = k_footstepInterval;
        }
    }
    else
    {
        // Reset so the first step right after starting to move plays immediately.
        m_footstepTimer = 0.0f;
    }
}

uint32_t EnemySound::postEvent(const char* eventName)
{
    if (m_source == nullptr || eventName == nullptr)
    {
        return 0;
    }
    return AudioAPI::postEvent(m_source, k_bank, eventName);
}

void EnemySound::postEventDelayed(const char* eventName, float delay)
{
    if (eventName == nullptr)
    {
        return;
    }
    if (delay <= 0.0f)
    {
        postEvent(eventName);
        return;
    }
    m_pendingEvents.push_back({ eventName, delay });
}

void EnemySound::playBasicTelegraph() { postEvent(evBasicTelegraph()); }
void EnemySound::playBasicImpact()    { postEvent(evBasicImpact()); }

void EnemySound::playHurt()
{
    if (m_hurtCooldownTimer > 0.0f)
    {
        return; // debounced: continuous/overlapping damage can't machine-gun the grunt
    }
    postEvent(evHurt());
    m_hurtCooldownTimer = k_hurtRetriggerCooldown;
}

void EnemySound::playStun()  { postEvent(evStun()); }
void EnemySound::playDeath() { postEvent(evDeath()); }

void EnemySound::notifyMoving()
{
    m_movingTimer = k_movingWatchdog;
}

void EnemySound::stopAllLoops()
{
    m_pendingEvents.clear();
    m_movingTimer   = 0.0f;
    m_footstepTimer = 0.0f;
}
