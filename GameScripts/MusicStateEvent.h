#pragma once

#include "ScriptAPI.h"
#include "GameplayEventAction.h"

class GameplayEventTrigger;

// Acción de evento de gameplay que cambia la MÚSICA. Mismo sistema que el resto de
// eventos (CombatAreaEvent, PopUpEvent, ...): va en el MISMO GameObject que un
// GameplayEventTrigger (con su collider TRIGGER). Cuando ambos jugadores entran en la
// zona, el trigger llama a executeEvent() -> setea m_stateOnEnter. Al salir llama a
// stopEvent() -> setea m_stateOnExit (si m_changeOnExit).
//
// Uso típico: capilla de Level 1 (enter Level1_Chapel / exit Level1_Upper) y ascensor
// de Level 2 (enter Level2_Elevator). El cambio de música lo ejecuta el MusicManager.
class MusicStateEvent : public GameplayEventAction
{
    DECLARE_SCRIPT(MusicStateEvent)

public:
    explicit MusicStateEvent(GameObject* owner);

    void executeEvent(GameplayEventTrigger* trigger) override;
    void stopEvent(GameplayEventTrigger* trigger) override;

    FieldList getExposedFields() const override;

    // Índices en kMusicStateNames (mismo dropdown que el MusicManager).
    int  m_stateOnEnter = 0;
    bool m_changeOnExit = false;
    int  m_stateOnExit  = 0;
};
