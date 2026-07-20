#pragma once

#include "ScriptAPI.h"

class Transform;

// Mantiene el SOUND_LISTENER en el punto medio de los dos jugadores (a ras de juego),
// en lugar de en la cámara. En un juego cenital la cámara está ~16+ unidades por encima
// de la acción, lo que empujaría TODO sonido 3D al extremo lejano de su curva de
// atenuación (bajísimo / inaudible). Colocando el listener en el midpoint, la distancia
// listener->emisor pasa a ser la distancia real de combate, y las ShareSets de
// atenuación funcionan tal como se diseñaron.
//
// Setup en escena:
//   1. GO dedicado (p.ej. "Audio Listener") con un componente SOUND_LISTENER + este script.
//   2. Cablear los dos transforms de los jugadores (Death / Lyriel).
//   3. QUITAR el SOUND_LISTENER de la cámara (solo debe haber uno).
class AudioListenerFollow : public Script
{
    DECLARE_SCRIPT(AudioListenerFollow)

public:
    explicit AudioListenerFollow(GameObject* owner);

    void Update() override;

    FieldList getExposedFields() const override;

    // Los dos jugadores. El listener se sitúa en su punto medio cada frame.
    ComponentRef<Transform> m_firstTarget;
    ComponentRef<Transform> m_secondTarget;

    // Orientación del listener. Por defecto = Fixed Rotation de la cámara, para que el
    // paneo 3D izquierda/derecha cuadre con la pantalla (enemigo a la izquierda → suena
    // por la izquierda). Si el paneo se siente raro, prueba [0, -45, 0] (listener nivelado).
    Vector3 m_listenerRotation{ 45.0f, -45.0f, 0.0f };

private:
    bool m_warnedMissing = false;
};
