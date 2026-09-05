#pragma once

#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

#include <string>
#include <vector>

class EnemyDetectionAggro;

class ArthurParticles final : public Script
{
    DECLARE_SCRIPT(ArthurParticles)

public:
    explicit ArthurParticles(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    void playChargingSlamImpact(const Vector3& position);
    void startEarthHammerShockwave();
    void stopEarthHammerShockwave();
    void playEarthHammerImpact(const Vector3& position);
    void playHeavySwipeHitsInCone(const Vector3& center, const Vector3& forward, float range, float halfAngleDegrees, int hitCount);

private:
    struct TimedEffect
    {
        enum class Type
        {
            GroundDust,
            ChargingSlam,
            HeavySwipeHit,
            ActivateShockwave,
            DeactivateShockwave
        };

        Type type = Type::GroundDust;
        float timer = 0.0f;
        Vector3 position = Vector3::Zero;
    };

    void scheduleEffect(TimedEffect::Type type, const Vector3& position, float delay);
    void processTimedEffects(float deltaTime);
    void spawnGroundDust(const Vector3& position);
    void spawnChargingSlam(const Vector3& position);
    void spawnHeavySwipeHit(const Vector3& position);
    void activateEarthHammerShockwave();
    void deactivateEarthHammerShockwave();

    bool isTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& forward, float range, float halfAngleDegrees) const;

    PrefabRef m_groundDustPrefab;
    PrefabRef m_chargingSlamPrefab;
    PrefabRef m_earthHammerShockwavePrefab;
    PrefabRef m_heavySwipePrefab;

    std::string m_groundDustPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Arthur/PS_BroundDustArthur.prefab";
    std::string m_chargingSlamPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Arthur/PS_ChargingSlam.prefab";
    std::string m_earthHammerShockwavePath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Arthur/PS_EarthHammerShockwave.prefab";
    std::string m_heavySwipePath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Arthur/PS_HeavySwipe.prefab";

    float m_chargingSlamGroundDustDelay = 0.0f;
    float m_chargingSlamExtraDelay = 0.0f;
    float m_earthHammerGroundDustDelay = 0.0f;
    float m_earthHammerShockwaveDelay = 0.0f;
    float m_heavySwipeHitDelay = 0.0f;

    GameObject* m_earthHammerShockwaveInstance = nullptr;
    bool m_earthHammerShockwaveActive = false;

    EnemyDetectionAggro* m_detectionAggro = nullptr;
    Transform* m_ownerTransform = nullptr;

    std::vector<TimedEffect> m_timedEffects;
    ParticleLifecycle::TimedParticleTracker m_timedOneShots;
};
