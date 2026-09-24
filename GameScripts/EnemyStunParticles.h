#pragma once
#include "ScriptAPI.h"

class Transform;
class Transform2D;

class EnemyStunParticles : public Script
{
    DECLARE_SCRIPT(EnemyStunParticles)
public:
    explicit EnemyStunParticles(GameObject* owner);
    void Start() override;
    void Update() override;
    void OnGameStop() override;
    FieldList getExposedFields() const override;

    PrefabRef m_stunPrefab;
    ComponentRef<Transform> m_stunAnchor;
    ComponentRef<Transform2D> m_stunIcon;
    float m_iconFadeTime = 0.15f;
    float       m_heightOffset = 2.0f;

    // Called by EnemyStunnedState
    void startStunParticle();
    void updateStunParticle();
    void stopStunParticle(bool immediate = false);

    // Destroys every runtime particle owned by this script. Safe to call more than once.
    void releaseRuntimeParticles();

private:
    void ensureStunParticle();
    Vector3 getStunParticlePosition() const;
    void resolveStunIcon();
    void applyStunIconAlpha();

    GameObject* m_stunParticle = nullptr;
    Transform* m_stunAnchorTransform = nullptr;
    Transform2D* m_stunIconTransform = nullptr;
    float m_iconAlpha = 0.0f;
    float m_iconTargetAlpha = 0.0f;
};
