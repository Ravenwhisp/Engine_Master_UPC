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
    ComponentRef<Transform2D> m_basicAttackUIBackground;
    ComponentRef<Transform2D> m_basicAttackUIGlow;

    Transform* m_basicAttackUICanvasTransform = nullptr;
    Transform2D* m_basicAttackUIBackgroundTransform2D = nullptr;
    Transform2D* m_basicAttackUIGlowTransform2D = nullptr;
};