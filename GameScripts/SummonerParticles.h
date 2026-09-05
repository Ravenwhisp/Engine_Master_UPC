#pragma once

#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

#include <string>

class SummonerParticles final : public Script
{
    DECLARE_SCRIPT(SummonerParticles)

public:
    explicit SummonerParticles(GameObject* owner);

    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    void playSummonParticle(const Vector3& position);
    void playTeleportParticle(const Vector3& position);

private:
    void spawnSummonParticle(const Vector3& position);
    void spawnTeleportBurst(const Vector3& position);

    Vector3 getOwnerRotation() const;

private:
    PrefabRef m_summonParticlePrefab;
    PrefabRef m_teleportParticlePrefab;

    std::string m_summonParticlePath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level2/Summoner/PS_Summoning.prefab";
    std::string m_teleportParticlePath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level2/Summoner/PS_Teleport.prefab";

    float m_summonParticleLifetime = 2.0f;
    float m_teleportDeactivateDelay = 2.0f;
    float m_summonYOffset = 0.0f;

    ParticleLifecycle::TimedParticleTracker m_timedParticles;
};
