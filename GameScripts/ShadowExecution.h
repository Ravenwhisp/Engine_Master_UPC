#pragma once

#include "ScriptAPI.h"
#include <vector>
#include "GameplayHaptics.h"

class ReaperGauge;
class DeathCharacter;
class LyrielCharacter;
class CooperativeSound;
class ShadowExecutionConfig;
class EnemyDamageable;

// Estructura para controlar el tiempo de vida de las partículas instanciadas
struct SpawnedPrefab {
    GameObject* gameObject;
    float lifetimeRemaining;
};

struct ShadowExecutionPreview
{
    float damage = 0.0f;
    float resultingHpPercent = 0.0f;
    bool willDie = false;
};

class ShadowExecution : public Script
{
    DECLARE_SCRIPT(ShadowExecution)

public:
    explicit ShadowExecution(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void OnGameStop() override;
    void drawGizmo() override;

    FieldList getExposedFields() const override;

    bool isActive() const { return m_isActive; }

    ShadowExecutionPreview calculatePreview(const EnemyDamageable* damageable) const;
    float getExecutionThresholdPercent(const EnemyDamageable* damageable) const;

public:
    AssetReference<ShadowExecutionConfig> m_config;

    PrefabRef m_particlePrefab;

private:
    void setupUI();
    void cachePlayers();
    void tryTrigger();
    void beginExecution();
    void updateExecution(float dt);
    void endExecution();
    void applyAoEDamage();
    void lockPlayers(bool locked);

    ShadowExecutionConfig* m_shadowExecutionConfig = nullptr;
    ReaperGauge* m_reaperGauge = nullptr;
    DeathCharacter* m_deathCharacter = nullptr;
    LyrielCharacter* m_lyrielCharacter = nullptr;
    CooperativeSound* m_sound = nullptr;

    float m_p0WindowTimer = 0.0f;
    float m_p1WindowTimer = 0.0f;

    bool    m_isActive = false;
    float   m_executionTimer = 0.0f;
    Vector3 m_center = Vector3::Zero;
    float   m_maxRadius = 0.0f;
    float   m_currentRadius = 0.0f;

    std::vector<GameObject*> m_hitEnemies;

    GameplayHapticRumble m_deathExecutionHaptic;
    GameplayHapticRumble m_lyrielExecutionHaptic;

    // Lista para trackear las partículas que deben morir tras 1 segundo
    std::vector<SpawnedPrefab> m_temporaryPrefabs;

public:
    ComponentRef<UISlider> m_reaperGaugeBar;
    ComponentRef<Transform2D> m_executionGlowUI;
    ComponentRef<Transform2D> m_glowUIplayer0;
    ComponentRef<Transform2D> m_glowUIplayer1;
    ComponentRef<Transform2D> m_controlsUIplayer0;
	ComponentRef<Transform2D> m_controlsUIplayer1;
    ComponentRef<Transform> m_executionCanvas;
    ComponentRef<Transform2D> m_executionSprite;

private:
    UISlider* m_reaperGaugeSlider = nullptr;
    Transform2D* m_executionGlowTransform = nullptr;
    Transform2D* m_glowTransformPlayer0 = nullptr;
    Transform2D* m_glowTransformPlayer1 = nullptr;
	Transform2D* m_controlsTransformPlayer0 = nullptr;
	Transform2D* m_controlsTransformPlayer1 = nullptr;
    Transform* m_executionTransform = nullptr;
    Transform2D* m_executionTransform2D = nullptr;

    void updateUI();
};
