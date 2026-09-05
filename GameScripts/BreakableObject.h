#pragma once

#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

class Transform;
class NavRuntimeBlockerComponent;

class BreakableObject : public Script
{
    DECLARE_SCRIPT(BreakableObject)

public:
    explicit BreakableObject(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnGameStop() override;

    virtual void onBreak();
    bool isBroken() const { return m_isBroken; }

    virtual bool canBeTargetedDuringCombat() const { return false; }

protected:
    Transform* m_normalObjectTransform = nullptr;
    Transform* m_brokenObjectTransform = nullptr;
    void breakObject();
    Vector3 getBreakEffectPosition() const;
    void spawnBreakBaseEffect();

    ParticleLifecycle::TimedParticleTracker m_timedBreakEffects;

private:
    bool m_isBroken = false;
    NavRuntimeBlockerComponent* m_navBlocker = nullptr;
};
