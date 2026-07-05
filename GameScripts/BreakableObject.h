#pragma once

#include "ScriptAPI.h"

class Transform;
class NavRuntimeBlockerComponent;

class BreakableObject : public Script
{
    DECLARE_SCRIPT(BreakableObject)

public:
    explicit BreakableObject(GameObject* owner);

    void Start() override;

    ScriptFieldList getExposedFields() const override;

    virtual void onBreak();
    bool isBroken() const { return m_isBroken; }

protected:
	Transform* m_normalObjectTransform = nullptr;
	Transform* m_brokenObjectTransform = nullptr;
    void breakObject();

public:
    std::string m_dustEffectParticle = "Assets/Prefabs/Particles/BarrelBreaking.prefab";

private:
    bool m_isBroken = false;
    NavRuntimeBlockerComponent* m_navBlocker = nullptr;

};