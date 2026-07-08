#pragma once

#include "ScriptAPI.h"
#include <vector>

struct ManagedParticle
{
    GameObject* gameObject           = nullptr;
    bool        deactivatedByManager = false;
};

class ParticleManager : public Script
{
    DECLARE_SCRIPT(ParticleManager)

public:
    explicit ParticleManager(GameObject* owner);

    void Start() override;
    void Update() override;

    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

public:
    ScriptComponentRef<Transform> m_playerTransform;
    float                         m_activationDistance   = 50.0f;
    float                         m_checkIntervalSeconds = 1.0f;

private:
    float                         m_timer = 0.0f;
    std::vector<ManagedParticle>  m_managedParticles;

    void refreshParticleCache();
    void updateActivity();
};
