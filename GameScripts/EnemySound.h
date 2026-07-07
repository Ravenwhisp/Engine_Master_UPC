#pragma once

#include "ScriptAPI.h"

#include <vector>

// Abstract base for enemy SFX (Paladin, Archer, ...). Holds all the machinery;
// subclasses only supply the Wwise event names (and any enemy-specific loops).
// The shared enemy states find it via findScript<EnemySound>(owner); enemy-specific
// states (charge, barrage) find the concrete subclass instead.
//
// All events live in the Level1 bank, which is 3D (attenuation set on the
// Enemies_ActorMixer). The GameObject must carry a SOUND_SOURCE component so Wwise
// has a moving emitter to compute distance against the listener.
class EnemySound : public Script
{
public:
    explicit EnemySound(GameObject* owner);

    void Start() override;
    void Update() override;

    // Called by the shared enemy states / damageable.
    void playBasicTelegraph();   // Paladin swing / Archer bow release (attack start)
    void playBasicImpact();      // weapon / arrow contact frame
    void playHurt();             // debounced one-shot
    void playStun();
    void playDeath();

    // Footsteps: the controller pings notifyMoving() every frame it actually steps;
    // a short watchdog turns the cadence off automatically once it stops moving, so
    // no state has to remember to silence them.
    void notifyMoving();

    // Stops every loop and pending delayed event. Base clears footsteps + pending;
    // subclasses override to also stop their own loops (e.g. the Paladin charge loop).
    virtual void stopAllLoops();

protected:
    // Event-name table — subclasses return string literals (nullptr = no such event).
    virtual const char* evBasicTelegraph() const = 0;
    virtual const char* evBasicImpact()    const = 0;
    virtual const char* evHurt()           const = 0;
    virtual const char* evStun()           const = 0;
    virtual const char* evDeath()          const = 0;
    virtual const char* evFootstep()       const = 0;

    uint32_t postEvent(const char* eventName);
    void     postEventDelayed(const char* eventName, float delay);

    ComponentSoundSource* m_source = nullptr;

private:
    struct PendingEvent
    {
        const char* eventName;
        float       timeRemaining;
    };
    std::vector<PendingEvent> m_pendingEvents;

    float m_hurtCooldownTimer = 0.0f;

    float m_movingTimer   = 0.0f;  // watchdog: > 0 while the enemy is locomoting
    float m_footstepTimer = 0.0f;  // cadence countdown
};
