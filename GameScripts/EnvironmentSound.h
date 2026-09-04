#pragma once

#include "ScriptAPI.h"

// Static helper to post LevelCommon.bnk events from a GameObject's SOUND_SOURCE.
// Interactables (breakables, crystals, doors, spikes, powerups...) use it so each plays
// its SFX positionally (3D) from its own emitter. The emitter GO must carry a SOUND_SOURCE.
class EnvironmentSound
{
public:
    // Posts a one-shot (or loop-start) event from the emitter's SOUND_SOURCE. Returns the
    // playingID. For loops, stop them by posting the matching Stop_ event. No-op (and a
    // warning) if the emitter has no SOUND_SOURCE.
    static uint32_t play(GameObject* emitter, const char* eventName);
};
