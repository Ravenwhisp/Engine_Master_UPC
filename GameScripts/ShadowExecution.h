#pragma once

#include "ScriptAPI.h"
#include <vector>

class ReaperGauge;
class DeathCharacter;
class LyrielCharacter;
class CooperativeSound;

// Estructura para controlar el tiempo de vida de las partículas instanciadas
struct SpawnedPrefab {
    GameObject* gameObject;
    float lifetimeRemaining;
};

class ShadowExecution : public Script
{
    DECLARE_SCRIPT(ShadowExecution)

public:
    explicit ShadowExecution(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

    bool isActive() const { return m_isActive; }

public:
    float m_timeWindow         = 2.0f;
    float m_executionDuration  = 3.0f;
    float m_instaKillThreshold = 0.20f;
    float m_standardDamage     = 0.10f;

    // Ruta del prefab de partículas (ajustable desde el editor si lo expones)
    const char* m_particlePrefabPath = "Assets/Prefabs/Particles/Death/ShadowExecution.prefab";

private:
    void cachePlayers();
    void tryTrigger();
    void beginExecution();
    void updateExecution(float dt);
    void endExecution();
    void applyAoEDamage();
    void lockPlayers(bool locked);

    ReaperGauge* m_reaperGauge     = nullptr;
    DeathCharacter* m_deathCharacter  = nullptr;
    LyrielCharacter* m_lyrielCharacter = nullptr;
    CooperativeSound* m_sound           = nullptr;

    float m_p0WindowTimer = 0.0f;
    float m_p1WindowTimer = 0.0f;

    bool    m_isActive        = false;
    float   m_executionTimer  = 0.0f;
    Vector3 m_center          = Vector3::Zero;
    float   m_maxRadius       = 0.0f;
    float   m_currentRadius   = 0.0f;

    std::vector<GameObject*> m_hitEnemies;
    
    // Lista para trackear las partículas que deben morir tras 1 segundo
    std::vector<SpawnedPrefab> m_temporaryPrefabs;

public:
    ScriptComponentRef<UISlider> m_reaperGaugeBar;
    ScriptComponentRef<Transform> m_executionCanvas;
    ScriptComponentRef<Transform2D> m_executionSprite;

private:
    UISlider* m_reaperGaugeSlider = nullptr;
    Transform* m_executionTransform = nullptr;
    Transform2D* m_executionTransform2D = nullptr;

    void updateUI();
};