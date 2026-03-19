#pragma once

#include "ScriptAPI.h"
#include "ScriptAutoRegister.h"
#include "ScriptFieldInfo.h"
#include "ScriptComponentRef.h"

class Transform;

class TriggerArea final : public Script
{
    DECLARE_SCRIPT(TriggerArea)

public:
    TriggerArea(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_xWidth = 2.0f;
    float m_zWidth = 2.0f;

    std::string m_sceneToLoad;

    ScriptComponentRef<Transform> m_firstTarget;
    ScriptComponentRef<Transform> m_secondTarget;

private:
    bool containsPoint(const Vector3& triggerCenter, const Vector3& point) const;
    void triggerSceneChange();
};