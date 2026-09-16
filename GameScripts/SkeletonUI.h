#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"

class SkeletonUI : public Script
{
    DECLARE_SCRIPT(SkeletonUI)

public:
    explicit SkeletonUI(GameObject* owner);

    void Start() override;

    FieldList getExposedFields() const override;

    void setupScimitarUI(float range, float halfAngleDegrees);

    void showScimitarTelegraph(
        const Vector3& origin,
        const Vector3& forwardDirection
    );

    void updateScimitarUIPose(
        const Vector3& origin,
        const Vector3& forwardDirection
    );

    void showScimitarImpact();
    void hideScimitarUI();

private:
    ComponentRef<Transform> m_scimitarUICanvas;
    ComponentRef<Transform2D> m_scimitarUITelegraph;
    ComponentRef<Transform2D> m_scimitarUIImpact;

    Transform* m_scimitarUICanvasTransform = nullptr;
    Transform2D* m_scimitarUITelegraphTransform2D = nullptr;
    Transform2D* m_scimitarUIImpactTransform2D = nullptr;

    float m_scimitarUIWidthMultiplier = 1.0f;
    float m_scimitarUILengthMultiplier = 1.0f;

    float m_scimitarUIForwardOffset = 0.0f;
    float m_scimitarUISideOffset = 0.0f;
    float m_scimitarUIHeightOffset = 0.05f;

    float m_scimitarUITelegraphAlpha = 0.45f;
    float m_scimitarUIImpactAlpha = 1.0f;

    float m_currentScimitarRange = 0.0f;
};