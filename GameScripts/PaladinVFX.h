#pragma once

#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

class PaladinVFX : public Script
{
    DECLARE_SCRIPT(PaladinVFX)

public:
    explicit PaladinVFX(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    void setWalkingDustActive(bool active);
    void stopWalkingDust();

    void startChargeAttackEffect();
    void stopChargeAttackEffect();

    void startBasicAttackTelegraph(
        const Vector3& position,
        const Vector3& rotation
    );

    void stopBasicAttackTelegraph();

    void playBasicAttackEffect();

    void playShieldAttackStart(const Vector3& position, const Vector3& direction);
    void playShieldAttackHits(const Vector3& origin, const Vector3& direction, float length, float width);

private:
    bool isTargetInRectangle(Transform* targetTransform, const Vector3& origin, const Vector3& direction, float length, float width) const;
    void spawnShieldAttackHit(const Vector3& position);

    Vector3 getWalkingDustPosition() const;
    Vector3 getOwnerRotation() const;
    Vector3 getChargeAttackEffectPosition() const;
    Vector3 getBasicAttackEffectPosition() const;

    void addWalkingDust();
    void removeWalkingDust();
    void updateWalkingDustPosition();

    void addChargeAttackEffect();
    void removeChargeAttackEffect();
    void updateChargeAttackEffectPosition();

    void addBasicAttackTelegraph(
        const Vector3& position,
        const Vector3& rotation
    );

    void removeBasicAttackTelegraph();

    void addBasicAttackEffect();
    void removeBasicAttackEffect();
    void updateBasicAttackEffectLifetime(float deltaTime);

    void ensureWalkingDust();
    void ensureChargeAttackEffect();
    void ensureBasicAttackTelegraph(const Vector3& position, const Vector3& rotation);
    void ensureBasicAttackEffect();

public:
    PrefabRef m_walkingDustPrefab;
    PrefabRef m_chargeAttackEffectPrefab;
    PrefabRef m_basicAttackEffectPrefab;
    PrefabRef m_shieldAttackParticlesPrefab;
    PrefabRef m_shieldAttackHitPrefab;

    std::string m_shieldAttackParticlesPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Melee/Shield/PS_ShieldAttackParticles.prefab";
    std::string m_shieldAttackHitPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Melee/PS_ShieldAttack.prefab";

    float walkingDustYOffset = 0.05f;
    float walkingDustForwardOffset = -0.35f;

private:
    GameObject* walkingDustEffect = nullptr;
    bool walkingDustActive = false;

    GameObject* chargeAttackEffect = nullptr;
    bool chargeAttackEffectActive = false;

    float chargeAttackYOffset = 0.5f;
    float chargeAttackForwardOffset = 0.0f;

    GameObject* basicAttackTelegraph = nullptr;
    float basicAttackTelegraphYOffset = 0.05f;

    GameObject* basicAttackEffect = nullptr;
    float basicAttackYOffset = 0.05f;
    float basicAttackForwardOffset = 0.75f;
    float basicAttackEffectLifetime = 1.0f;
    float basicAttackEffectTimer = 0.0f;

    class EnemyDetectionAggro* m_detectionAggro = nullptr;
    float m_shieldAttackParticlesYOffset = 0.05f;

    ParticleLifecycle::TimedParticleTracker m_timedHitVfx;
};
