#pragma once

#include "ScriptAPI.h"

#include <string>

// Plays a looping ambient event on Start from this GameObject's SOUND_SOURCE
// (e.g. a fire crackle on a torch/bonfire). Generic: the play/stop event names are
// serialized, so the same script serves fire now and water/wind later.
//
// Setup: put it on the ambient emitter GO (which needs a SOUND_SOURCE) and set the
// events. Defaults to the fire loop. The loop persists for the scene; on scene change the
// engine's StopAll cuts it. Call stop() if the source is extinguished mid-scene.
class AmbientSoundLoop : public Script
{
    DECLARE_SCRIPT(AmbientSoundLoop)

public:
    explicit AmbientSoundLoop(GameObject* owner);

    void Start() override;

    FieldList getExposedFields() const override;

    void stop();

    std::string m_playEvent   = "Play_Ambient_Fire_Crack";
    std::string m_stopEvent   = "Stop_Ambient_Fire_Crack";
    bool        m_playOnStart = true;

private:
    ComponentSoundSource* m_source    = nullptr;
    uint32_t              m_playingID = 0;
};
