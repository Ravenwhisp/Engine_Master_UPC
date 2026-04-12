#include "pch.h"
#include "TransformGlobalTest.h"

static const ScriptFieldInfo transformGlobalTestFields[] =
{
    { "Run SetGlobalPosition Test", ScriptFieldType::Bool, offsetof(TransformGlobalTest, m_runSetGlobalPositionTest) },
    { "Run TranslateGlobal Test", ScriptFieldType::Bool, offsetof(TransformGlobalTest, m_runTranslateGlobalTest) },
    { "Run SetGlobalRotation Test", ScriptFieldType::Bool, offsetof(TransformGlobalTest, m_runSetGlobalRotationTest) },
    { "Run LookAt Test", ScriptFieldType::Bool, offsetof(TransformGlobalTest, m_runLookAtTest) },

    { "Target Global Position", ScriptFieldType::Vec3, offsetof(TransformGlobalTest, m_targetGlobalPosition) },
    { "Translate Delta", ScriptFieldType::Vec3, offsetof(TransformGlobalTest, m_translateDelta) },
    { "Target Global Euler", ScriptFieldType::Vec3, offsetof(TransformGlobalTest, m_targetGlobalEuler) },
    { "LookAt Target World Position", ScriptFieldType::Vec3, offsetof(TransformGlobalTest, m_lookAtTargetWorldPosition) }
};

IMPLEMENT_SCRIPT_FIELDS(TransformGlobalTest, transformGlobalTestFields)

TransformGlobalTest::TransformGlobalTest(GameObject* owner)
    : Script(owner)
{
}

void TransformGlobalTest::Start()
{
    Transform* transform = GameObjectAPI::getTransform(getOwner());
    if (transform == nullptr)
    {
        Debug::log("[TransformGlobalTest] No transform found.");
        return;
    }

    const Vector3 localPos = TransformAPI::getPosition(transform);
    const Vector3 globalPos = TransformAPI::getGlobalPosition(transform);
    const Vector3 localEuler = TransformAPI::getEulerDegrees(transform);
    const Vector3 globalEuler = TransformAPI::getGlobalEulerDegrees(transform);

    Debug::log("[TransformGlobalTest] START - Object: %s | Local Pos: (%.2f, %.2f, %.2f) | Global Pos: (%.2f, %.2f, %.2f) | Local Euler: (%.2f, %.2f, %.2f) | Global Euler: (%.2f, %.2f, %.2f)",
        GameObjectAPI::getName(getOwner()),
        localPos.x, localPos.y, localPos.z,
        globalPos.x, globalPos.y, globalPos.z,
        localEuler.x, localEuler.y, localEuler.z,
        globalEuler.x, globalEuler.y, globalEuler.z);

    m_loggedInitialState = true;
}

void TransformGlobalTest::Update()
{
    Transform* transform = GameObjectAPI::getTransform(getOwner());
    if (transform == nullptr)
    {
        return;
    }

    if (m_runSetGlobalPositionTest)
    {
        Debug::log("[TransformGlobalTest] Running setGlobalPosition to (%.2f, %.2f, %.2f)",
            m_targetGlobalPosition.x, m_targetGlobalPosition.y, m_targetGlobalPosition.z);

        TransformAPI::setGlobalPosition(transform, m_targetGlobalPosition);

        const Vector3 localPos = TransformAPI::getPosition(transform);
        const Vector3 globalPos = TransformAPI::getGlobalPosition(transform);

        Debug::log("[TransformGlobalTest] AFTER setGlobalPosition | Local Pos: (%.2f, %.2f, %.2f) | Global Pos: (%.2f, %.2f, %.2f)",
            localPos.x, localPos.y, localPos.z,
            globalPos.x, globalPos.y, globalPos.z);

        m_runSetGlobalPositionTest = false;
    }

    if (m_runTranslateGlobalTest)
    {
        Debug::log("[TransformGlobalTest] Running translateGlobal by (%.2f, %.2f, %.2f)",
            m_translateDelta.x, m_translateDelta.y, m_translateDelta.z);

        TransformAPI::translateGlobal(transform, m_translateDelta);

        const Vector3 localPos = TransformAPI::getPosition(transform);
        const Vector3 globalPos = TransformAPI::getGlobalPosition(transform);

        Debug::log("[TransformGlobalTest] AFTER translateGlobal | Local Pos: (%.2f, %.2f, %.2f) | Global Pos: (%.2f, %.2f, %.2f)",
            localPos.x, localPos.y, localPos.z,
            globalPos.x, globalPos.y, globalPos.z);

        m_runTranslateGlobalTest = false;
    }

    if (m_runSetGlobalRotationTest)
    {
        Debug::log("[TransformGlobalTest] Running setGlobalRotationEuler to (%.2f, %.2f, %.2f)",
            m_targetGlobalEuler.x, m_targetGlobalEuler.y, m_targetGlobalEuler.z);

        TransformAPI::setGlobalRotationEuler(transform, m_targetGlobalEuler);

        const Vector3 localEuler = TransformAPI::getEulerDegrees(transform);
        const Vector3 globalEuler = TransformAPI::getGlobalEulerDegrees(transform);
        const Vector3 forward = TransformAPI::getForward(transform);

        Debug::log("[TransformGlobalTest] AFTER setGlobalRotationEuler | Local Euler: (%.2f, %.2f, %.2f) | Global Euler: (%.2f, %.2f, %.2f) | Forward: (%.2f, %.2f, %.2f)",
            localEuler.x, localEuler.y, localEuler.z,
            globalEuler.x, globalEuler.y, globalEuler.z,
            forward.x, forward.y, forward.z);

        m_runSetGlobalRotationTest = false;
    }

    if (m_runLookAtTest)
    {
        Debug::log("[TransformGlobalTest] Running lookAt to target world position (%.2f, %.2f, %.2f)",
            m_lookAtTargetWorldPosition.x, m_lookAtTargetWorldPosition.y, m_lookAtTargetWorldPosition.z);

        TransformAPI::lookAt(transform, m_lookAtTargetWorldPosition);

        const Vector3 localEuler = TransformAPI::getEulerDegrees(transform);
        const Vector3 globalEuler = TransformAPI::getGlobalEulerDegrees(transform);
        const Vector3 globalPos = TransformAPI::getGlobalPosition(transform);
        const Vector3 forward = TransformAPI::getForward(transform);

        Vector3 expectedDir = m_lookAtTargetWorldPosition - globalPos;
        if (expectedDir.LengthSquared() > 0.0001f)
        {
            expectedDir.Normalize();
        }

        Debug::log("[TransformGlobalTest] AFTER lookAt | Local Euler: (%.2f, %.2f, %.2f) | Global Euler: (%.2f, %.2f, %.2f) | Forward: (%.2f, %.2f, %.2f) | Expected Dir: (%.2f, %.2f, %.2f)",
            localEuler.x, localEuler.y, localEuler.z,
            globalEuler.x, globalEuler.y, globalEuler.z,
            forward.x, forward.y, forward.z,
            expectedDir.x, expectedDir.y, expectedDir.z);

        m_runLookAtTest = false;
    }
}

IMPLEMENT_SCRIPT(TransformGlobalTest)