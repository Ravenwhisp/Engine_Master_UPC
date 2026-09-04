#pragma once

#include "ScriptAPI.h"

class CrystalVisuals : public Script
{
    DECLARE_SCRIPT(CrystalVisuals)

public:
    explicit CrystalVisuals(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

    void setActivated(bool activated);
    bool isActivated() const { return m_activated; }

private:
    void resolveReferences();
    void updateRotation(float deltaTime);
    void updateModelVisibility();

public:
    ComponentRef<Transform> m_rotationPivot;
    ComponentRef<Transform> m_inactiveModel;
    ComponentRef<Transform> m_activeModel;

    float m_rotationSpeed = 30.0f;

private:
    Transform* m_rotationPivotTransform = nullptr;
    Transform* m_inactiveModelTransform = nullptr;
    Transform* m_activeModelTransform = nullptr;

    bool m_activated = false;
};