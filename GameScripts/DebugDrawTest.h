#pragma once

#include "ScriptAPI.h"

class DebugDrawTest : public Script
{
    DECLARE_SCRIPT(DebugDrawTest)

public:
    explicit DebugDrawTest(GameObject* owner);

    void Start() override;
    void Update() override;

    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

public:
    Vector3 m_origin = { 0.0f, 0.0f, 0.0f };
    float m_axisSize = 0.5f;
    float m_axisLength = 1.0f;
    float m_circleRadius = 1.0f;
    float m_circleSteps = 16.0f;
    Vector3 m_arrowEnd = { 2.0f, 1.0f, 0.0f };
    float m_arrowSize = 0.2f;
    float m_sphereRadius = 0.5f;
    float m_boxSize = 1.0f;
    Vector3 m_coneApex = { 0.0f, 1.0f, 0.0f };
    Vector3 m_coneDir = { 0.0f, -1.0f, 0.0f };
    float m_coneBaseRadius = 0.5f;
    float m_vertexNormalLength = 0.5f;
    float m_gridExtent = 10.0f;
    float m_gridStep = 1.0f;
    float m_gridY = 0.0f;
    int m_duration = 0;
    bool m_depthEnabled = true;
    float m_textScaling = 1.0f;
};