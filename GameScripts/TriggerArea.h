#pragma once

#include "ScriptAPI.h"

#include <string>

class Transform;

class TriggerArea final : public Script
{
    DECLARE_SCRIPT(TriggerArea)

public:
    TriggerArea(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

public:
    float m_xWidth = 2.0f;
    float m_zWidth = 2.0f;

    std::string m_sceneToLoad;

    ComponentRef<Transform> m_firstTarget;
    ComponentRef<Transform> m_secondTarget;

private:
    bool containsPoint(const Vector3& triggerCenter, const Vector3& point) const;
    void triggerSceneChange();
};