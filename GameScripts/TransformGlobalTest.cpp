#include "pch.h"
#include "TransformGlobalTest.h"

static const ScriptFieldInfo transformGlobalTestFields[] =
{
    { "Run SetGlobalPosition Test", ScriptFieldType::Bool, offsetof(TransformGlobalTest, m_runSetGlobalPositionTest) },
    { "Run TranslateGlobal Test", ScriptFieldType::Bool, offsetof(TransformGlobalTest, m_runTranslateGlobalTest) },
    { "Target Global Position", ScriptFieldType::Vec3, offsetof(TransformGlobalTest, m_targetGlobalPosition) },
    { "Translate Delta", ScriptFieldType::Vec3, offsetof(TransformGlobalTest, m_translateDelta) }
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

    Debug::log("[TransformGlobalTest] START - Object: %s | Local Pos: (%.2f, %.2f, %.2f) | Global Pos: (%.2f, %.2f, %.2f)",
        GameObjectAPI::getName(getOwner()),
        localPos.x, localPos.y, localPos.z,
        globalPos.x, globalPos.y, globalPos.z);

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
}

IMPLEMENT_SCRIPT(TransformGlobalTest)