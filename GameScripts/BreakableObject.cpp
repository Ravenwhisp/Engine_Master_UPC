#include "pch.h"
#include "BreakableObject.h"

BreakableObject::BreakableObject(GameObject* owner)
    : Script(owner)
{
}

void BreakableObject::Start()
{
    m_isBroken = false;

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    m_normalObjectTransform = TransformAPI::findChildByName(ownerTransform, "Normal");
    m_brokenObjectTransform = TransformAPI::findChildByName(ownerTransform, "Broken");

    if (m_normalObjectTransform == nullptr)
    {
        Debug::warn("[BreakableObject] '%s' could not find child object named 'Normal'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_brokenObjectTransform == nullptr)
    {
        Debug::warn("[BreakableObject] '%s' could not find child object named 'Broken'.", GameObjectAPI::getName(getOwner()));
    }

    if (m_normalObjectTransform != nullptr)
    {
        GameObject* normalObject = ComponentAPI::getOwner(m_normalObjectTransform);
        GameObjectAPI::setActive(normalObject, true);
    }

    if (m_brokenObjectTransform != nullptr)
    {
        GameObject* brokenObject = ComponentAPI::getOwner(m_brokenObjectTransform);
        GameObjectAPI::setActive(brokenObject, false);
    }
}

void BreakableObject::breakObject()
{
    if (m_isBroken)
    {
        return;
    }

    m_isBroken = true;

    if (m_normalObjectTransform != nullptr)
    {
        GameObject* normalObject = ComponentAPI::getOwner(m_normalObjectTransform);
        GameObjectAPI::setActive(normalObject, false);
    }

    if (m_brokenObjectTransform != nullptr)
    {
        GameObject* brokenObject = ComponentAPI::getOwner(m_brokenObjectTransform);
        GameObjectAPI::setActive(brokenObject, true);
    }

    Debug::log("[BreakableObject] '%s' broke.", GameObjectAPI::getName(getOwner()));
}

IMPLEMENT_SCRIPT(BreakableObject)