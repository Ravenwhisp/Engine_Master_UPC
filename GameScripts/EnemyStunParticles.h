#pragma once
#include "ScriptAPI.h"
#include <string>

class EnemyStunParticles : public Script
{
    DECLARE_SCRIPT(EnemyStunParticles)
public:
    explicit EnemyStunParticles(GameObject* owner);
    void Start() override;
    ScriptFieldList getExposedFields() const override;

    PrefabRef m_stunPrefab;
    float       m_heightOffset = 2.0f;

    // Called by EnemyStunnedState
    void startStunParticle();
    void updateStunParticle();
    void stopStunParticle();

private:
    GameObject* m_stunParticle = nullptr;
};
