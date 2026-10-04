#pragma once

#include "ScriptAPI.h"
#include "ProjectileBase.h"

class LyrielParticles;

class LyrielArrowProjectile : public ProjectileBase
{
    DECLARE_SCRIPT(LyrielArrowProjectile)

public:
    explicit LyrielArrowProjectile(GameObject* owner);

    void Update() override;
    FieldList getExposedFields() const override;

    enum class VisualModel
    {
        Basic = 0,
        Charged = 1,
        Volley = 2
    };

    void launch(const Vector3& startPosition, const Vector3& direction, float speed, float lifetime, GameObject* target, float damage, VisualModel visual = VisualModel::Basic);

    void resetProjectile() override;

private:
    void applyImpactDamage();
    void syncParticleTransform();
    void activateEmbeddedParticles();
    void stopEmbeddedParticles();

    LyrielParticles* getLyrielParticles() const;

public:
    std::string m_legacyParticlePath;
    PrefabRef m_particlePrefab;

	// visual prefabs for different arrow types
    PrefabRef m_visualBasicPrefab;
    PrefabRef m_visualChargedPrefab;
    PrefabRef m_visualVolleyPrefab;

private:
    Vector3 m_direction = Vector3::Zero;

    float m_speed = 0.0f;
    float m_currentLifetime = 0.0f;
    float m_lifeTimer = 0.0f;

    GameObject* m_target = nullptr;
    float m_damage = 0.0f;

    GameObject* m_particleGO = nullptr;
    GameObject* m_visualGO = nullptr; 
};