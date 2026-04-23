#include "pch.h"
#include "DebugDrawTest.h"

IMPLEMENT_SCRIPT_FIELDS(DebugDrawTest,
    SERIALIZED_VEC3(m_origin, "Origin"),
    SERIALIZED_FLOAT(m_axisSize, "Axis Size", 0.01f, 10.0f, 0.01f),
    SERIALIZED_FLOAT(m_axisLength, "Axis Length", 0.01f, 10.0f, 0.01f),
    SERIALIZED_FLOAT(m_circleRadius, "Circle Radius", 0.01f, 50.0f, 0.05f),
    SERIALIZED_FLOAT(m_circleSteps, "Circle Steps", 3.0f, 64.0f, 1.0f),
    SERIALIZED_VEC3(m_arrowEnd, "Arrow End"),
    SERIALIZED_FLOAT(m_arrowSize, "Arrow Size", 0.01f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_sphereRadius, "Sphere Radius", 0.01f, 50.0f, 0.05f),
    SERIALIZED_FLOAT(m_boxSize, "Box Size", 0.01f, 50.0f, 0.05f),
    SERIALIZED_VEC3(m_coneApex, "Cone Apex"),
    SERIALIZED_VEC3(m_coneDir, "Cone Direction"),
    SERIALIZED_FLOAT(m_coneBaseRadius, "Cone Base Radius", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_vertexNormalLength, "Vertex Normal Length", 0.01f, 10.0f, 0.01f),
    SERIALIZED_FLOAT(m_gridExtent, "Grid Extent", 1.0f, 100.0f, 1.0f),
    SERIALIZED_FLOAT(m_gridStep, "Grid Step", 0.1f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_gridY, "Grid Y", -10.0f, 10.0f, 0.1f),
    SERIALIZED_INT(m_duration, "Duration (ms)"),
    SERIALIZED_BOOL(m_depthEnabled, "Depth Enabled"),
    SERIALIZED_FLOAT(m_textScaling, "Text Scaling", 0.1f, 10.0f, 0.1f)
)

DebugDrawTest::DebugDrawTest(GameObject* owner)
    : Script(owner)
{
}

void DebugDrawTest::Start()
{
}

void DebugDrawTest::Update()
{

}

void DebugDrawTest::drawGizmo()
{
    using namespace DebugDrawAPI;

    const Vector3 red = { 1.0f, 0.0f, 0.0f };
    const Vector3 green = { 0.0f, 1.0f, 0.0f };
    const Vector3 blue = { 0.0f, 0.0f, 1.0f };
    const Vector3 white = { 1.0f, 1.0f, 1.0f };
    const Vector3 yellow = { 1.0f, 1.0f, 0.0f };
    const Vector3 cyan = { 0.0f, 1.0f, 1.0f };
    const Vector3 magenta = { 1.0f, 0.0f, 1.0f };

    drawPoint(m_origin, white, 4.0f, m_duration, m_depthEnabled);

    drawLine(m_origin, m_origin + Vector3(1.0f, 0.0f, 0.0f), red, m_duration, m_depthEnabled);
    drawLine(m_origin, m_origin + Vector3(0.0f, 1.0f, 0.0f), green, m_duration, m_depthEnabled);
    drawLine(m_origin, m_origin + Vector3(0.0f, 0.0f, 1.0f), blue, m_duration, m_depthEnabled);

    Matrix identity = Matrix(
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
    drawAxisTriad(identity, m_axisSize, m_axisLength, m_duration, m_depthEnabled);

    drawArrow(m_origin, m_arrowEnd, yellow, m_arrowSize, m_duration, m_depthEnabled);

    drawCross(m_origin, 0.5f, m_duration, m_depthEnabled);

    drawCircle(m_origin + Vector3(m_circleRadius + 0.2f, 0.0f, 0.0f), Vector3(0.f, 1.f, 0.f), red, m_circleRadius, m_circleSteps, m_duration, m_depthEnabled);

    drawPlane(m_origin + Vector3(0.0f, 0.1f, m_circleRadius + 0.2f), Vector3(0.f, 1.f, 0.f), blue, red, 2.0f, 0.5f, m_duration, m_depthEnabled);

    drawSphere(m_origin + Vector3(-m_circleRadius - 0.2f, m_sphereRadius, 0.0f), green, m_sphereRadius, m_duration, m_depthEnabled);

    drawCone(m_coneApex, m_coneDir, cyan, m_coneBaseRadius, 0.0f, m_duration, m_depthEnabled);

    drawBox(m_origin + Vector3(3.0f, 0.0f, 0.0f), yellow, m_boxSize, m_boxSize, m_boxSize, m_duration, m_depthEnabled);

    Vector3 aabbMin = m_origin + Vector3(4.5f, -0.5f, -0.5f);
    Vector3 aabbMax = m_origin + Vector3(5.5f, 0.5f, 0.5f);
    drawAABB(aabbMin, aabbMax, magenta, m_duration, m_depthEnabled);

    drawXZSquareGrid(-m_gridExtent, m_gridExtent, m_gridY, m_gridStep, white, m_duration, m_depthEnabled);

    drawScreenText("Debug Draw Test", { 100.0f, 100.0f, 0.0f }, yellow, m_textScaling, m_duration);

    const Vector3 tangentBasisOrigin = m_origin + Vector3(0.0f, 0.0f, 3.0f);
    const Vector3 n = Vector3(0.f, 1.f, 0.f);
    const Vector3 t = Vector3(1.f, 0.f, 0.f);
    const Vector3 b = Vector3(0.f, 0.f, -1.f);
    drawTangentBasis(tangentBasisOrigin, n, t, b, m_vertexNormalLength, m_duration, m_depthEnabled);

    drawVertexNormal(tangentBasisOrigin + Vector3(0.5f, 0.0f, 0.0f), Vector3(0.f, 1.f, 0.f), m_vertexNormalLength, m_duration, m_depthEnabled);
}

IMPLEMENT_SCRIPT(DebugDrawTest)