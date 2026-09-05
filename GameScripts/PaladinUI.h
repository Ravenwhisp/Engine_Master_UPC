#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"

class PaladinUI : public Script
{
    DECLARE_SCRIPT(PaladinUI)

public:
    explicit PaladinUI(GameObject* owner);

    void Start() override;

    FieldList getExposedFields() const override;

    void setupBasicAttackUI(float width, float length);

    void showBasicAttackUI(
        const Vector3& centerPosition,
        const Vector3& forwardDirection
    );

    void updateBasicAttackUIPose(
        const Vector3& centerPosition,
        const Vector3& forwardDirection
    );

    void showBasicAttackImpact();
    void hideBasicAttackUI();

private:
    ComponentRef<Transform> m_basicAttackUICanvas;
    ComponentRef<Transform2D> m_basicAttackUITelegraph;
    ComponentRef<Transform2D> m_basicAttackUIImpact;

    Transform* m_basicAttackUICanvasTransform = nullptr;
    Transform2D* m_basicAttackUITelegraphTransform2D = nullptr;
    Transform2D* m_basicAttackUIImpactTransform2D = nullptr;

    float m_basicAttackUIWidthMultiplier = 1.0f;
    float m_basicAttackUILengthMultiplier = 1.0f;

    float m_basicAttackUIForwardOffset = 0.0f;
    float m_basicAttackUISideOffset = 0.0f;
    float m_basicAttackUIHeightOffset = 0.05f;

    float m_basicAttackUITelegraphAlpha = 0.45f;
    float m_basicAttackUIImpactAlpha = 1.0f;
};