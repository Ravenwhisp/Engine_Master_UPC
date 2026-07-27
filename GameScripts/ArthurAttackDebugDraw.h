#pragma once

#include "ScriptAPI.h"

class ArthurAttackConfig;

class ArthurAttackDebugDraw : public Script
{
    DECLARE_SCRIPT(ArthurAttackDebugDraw)

public:
    explicit ArthurAttackDebugDraw(GameObject* owner);

    void Start() override;
    void drawGizmo() override;

    FieldList getExposedFields() const override;

public:
    bool m_debugEnabled = true;
    bool m_drawHeavySwipe = true;
    bool m_drawSideSweepLeft = true;
    bool m_drawSideSweepRight = true;
    bool m_drawEarthHammer = true;
    bool m_drawChargingSlam = true;

    float m_heightOffset = 0.15f;

private:
    AssetReference<ArthurAttackConfig> m_attackConfig;

private:
    void drawHeavySwipeCone() const;
    void drawSideSweepCone(int side) const;
    void drawEarthHammerRadius() const;
    void drawChargingSlamPreview() const;

    Vector3 rotateAroundY(const Vector3& vector, float radians) const;
};