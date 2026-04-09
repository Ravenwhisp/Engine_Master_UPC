#include "pch.h"
#include "DebugDrawTest.h"

static const ScriptFieldInfo debugDrawTestFields[] =
{
    { "Origin", ScriptFieldType::Vec3, offsetof(DebugDrawTest, m_origin) },
    { "Axis Size", ScriptFieldType::Float, offsetof(DebugDrawTest, m_axisSize), { 0.01f, 10.0f, 0.01f } },
    { "Axis Length", ScriptFieldType::Float, offsetof(DebugDrawTest, m_axisLength), { 0.01f, 10.0f, 0.01f } },
    { "Circle Radius", ScriptFieldType::Float, offsetof(DebugDrawTest, m_circleRadius), { 0.01f, 50.0f, 0.05f } },
    { "Circle Steps", ScriptFieldType::Float, offsetof(DebugDrawTest, m_circleSteps), { 3.0f, 64.0f, 1.0f } },
    { "Arrow End", ScriptFieldType::Vec3, offsetof(DebugDrawTest, m_arrowEnd) },
    { "Arrow Size", ScriptFieldType::Float, offsetof(DebugDrawTest, m_arrowSize), { 0.01f, 5.0f, 0.01f } },
    { "Sphere Radius", ScriptFieldType::Float, offsetof(DebugDrawTest, m_sphereRadius), { 0.01f, 50.0f, 0.05f } },
    { "Box Size", ScriptFieldType::Float, offsetof(DebugDrawTest, m_boxSize), { 0.01f, 50.0f, 0.05f } },
    { "Cone Apex", ScriptFieldType::Vec3, offsetof(DebugDrawTest, m_coneApex) },
    { "Cone Direction", ScriptFieldType::Vec3, offsetof(DebugDrawTest, m_coneDir) },
    { "Cone Base Radius", ScriptFieldType::Float, offsetof(DebugDrawTest, m_coneBaseRadius), { 0.0f, 10.0f, 0.05f } },
    { "Vertex Normal Length", ScriptFieldType::Float, offsetof(DebugDrawTest, m_vertexNormalLength), { 0.01f, 10.0f, 0.01f } },
    { "Grid Extent", ScriptFieldType::Float, offsetof(DebugDrawTest, m_gridExtent), { 1.0f, 100.0f, 1.0f } },
    { "Grid Step", ScriptFieldType::Float, offsetof(DebugDrawTest, m_gridStep), { 0.1f, 10.0f, 0.1f } },
    { "Grid Y", ScriptFieldType::Float, offsetof(DebugDrawTest, m_gridY), { -10.0f, 10.0f, 0.1f } },
    { "Duration (ms)", ScriptFieldType::Int, offsetof(DebugDrawTest, m_duration), { 0.0f, 60000.0f, 100.0f } },
    { "Depth Enabled", ScriptFieldType::Bool, offsetof(DebugDrawTest, m_depthEnabled) },
    { "Text Scaling", ScriptFieldType::Float, offsetof(DebugDrawTest, m_textScaling), { 0.1f, 10.0f, 0.1f } },
};

IMPLEMENT_SCRIPT_FIELDS(DebugDrawTest, debugDrawTestFields)

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