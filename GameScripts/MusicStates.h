#pragma once

// Nombres EXACTOS de los States del State Group "MusicState" de Wwise.
// Fuente autoritativa: Engine/Assets/Audio/MusicBoundByDeath.json
// El orden define el índice usado por los dropdowns (SERIALIZED_ENUM_INT)
// en MusicManager (m_sceneBaseState) y MusicStateEvent (m_stateOnEnter / m_stateOnExit).
//
// `static` => enlace interno por unidad de traducción (sin violar la ODR si se
// incluye en varios .cpp). Es el mismo patrón que usan otros enums del proyecto.
static const char* kMusicStateNames[] =
{
    "None",            // 0 - silencio (default)
    "MainMenu",        // 1
    "Level1_Upper",    // 2
    "Level1_Chapel",   // 3
    "Level1_Boss",     // 4 - Arthur
    "Level2_Upper",    // 5
    "Level2_Elevator", // 6
    "FinalBoss"        // 7 - Nivel 3 (escena aún no existe)
};

constexpr int kMusicStateCount = 8;
