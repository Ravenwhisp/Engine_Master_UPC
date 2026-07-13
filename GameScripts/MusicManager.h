#pragma once

#include "ScriptAPI.h"

// Gestor central de la MÚSICA adaptativa (Wwise). Mismo rol que DeathSound/
// LyrielSound/CooperativeSound para los SFX: TODAS las definiciones viven aquí;
// el resto de scripts/eventos SOLO llaman a estos métodos, nunca tocan Wwise directo.
//
// La música es 2D (no posicional): vive en un GameObject global con SOUND_SOURCE
// (recomendado: el mismo GameController que CooperativeSound).
//
// Modelo Wwise (ver MusicBoundByDeath.json):
//   - Eventos: Play_Music / Stop_Music (sobre MUS_Main).
//   - El cambio de canción NO es un evento: se setea el State Group "MusicState"
//     y Wwise transiciona solo (crossfade horneado en el bank).
//
// IMPORTANTE: el AudioAPI del engine todavía NO expone SetState de Wwise, así que
// SetMusicState() deja la llamada real comentada + un Debug::log provisional.
// Cuando el engine añada AudioAPI::setState => descomentar 1 línea (ver el .cpp).
class MusicManager : public Script
{
    DECLARE_SCRIPT(MusicManager)

public:
    explicit MusicManager(GameObject* owner);

    void Start() override;

    FieldList getExposedFields() const override;

    // --- Único punto que arranca / para la música ---
    void PlayMusic();
    void StopMusic();

    // --- Cambio de canción (cambia el State Group "MusicState") ---
    void SetMusicState(const char* stateValue);

    // Helpers legibles por contexto (lo que llaman otros scripts/eventos):
    void SetState_None();
    void SetState_MainMenu();
    void SetState_Level1Upper();
    void SetState_Level1Chapel();
    void SetState_Level1Boss();
    void SetState_Level2Upper();
    void SetState_Level2Elevator();
    void SetState_FinalBoss();

    // Acceso global: cualquier script hace MusicManager::Get()->SetState_...()
    // sin necesidad de cachear una referencia. Puede ser nullptr si la escena
    // no tiene MusicManager o aún no ha corrido su Start(): SIEMPRE null-check.
    static MusicManager* Get() { return s_instance; }

    // Campos serializados (public por las macros offsetof).
    int  m_sceneBaseState   = 0;     // índice en kMusicStateNames; estado al cargar la escena
    bool m_playMusicOnStart = true;  // (re)arrancar la música al cargar esta escena

private:
    uint32_t postEvent(const char* eventName);

    ComponentSoundSource* m_source        = nullptr;
    uint32_t              m_musicPlayingId = 0;

    static MusicManager* s_instance;
};
