#include "pch.h"
#include "TriggerArea.h"
#include "ScriptAPI.h"

static const ScriptFieldInfo triggerAreaFields[] =
{
    { "X Width", ScriptFieldType::Float, offsetof(TriggerArea, m_xWidth), { 0.0f, 1000.0f, 0.1f } },
    { "Z Width", ScriptFieldType::Float, offsetof(TriggerArea, m_zWidth), { 0.0f, 1000.0f, 0.1f } },
    { "Scene To Load", ScriptFieldType::String, offsetof(TriggerArea, m_sceneToLoad) },
    { "First Target", ScriptFieldType::ComponentRef, offsetof(TriggerArea, m_firstTarget), {}, {}, { ComponentType::TRANSFORM } },
    { "Second Target", ScriptFieldType::ComponentRef, offsetof(TriggerArea, m_secondTarget), {}, {}, { ComponentType::TRANSFORM } }
};

IMPLEMENT_SCRIPT_FIELDS(TriggerArea, triggerAreaFields)

TriggerArea::TriggerArea(GameObject* owner)
    : Script(owner)
{
}

void TriggerArea::Start()
{
}

void TriggerArea::Update()
{
    GameObject* owner = getOwner();
    if (!owner)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(owner);
    if (!ownerTransform)
    {
        return;
    }

    const Vector3 triggerCenter = TransformAPI::getPosition(ownerTransform);

    Transform* firstTarget = m_firstTarget.getReferencedComponent();
    if (firstTarget && containsPoint(triggerCenter, TransformAPI::getPosition(firstTarget)))
    {
        triggerSceneChange();
        return;
    }

    Transform* secondTarget = m_secondTarget.getReferencedComponent();
    if (secondTarget && containsPoint(triggerCenter, TransformAPI::getPosition(secondTarget)))
    {
        triggerSceneChange();
    }
}

bool TriggerArea::containsPoint(const Vector3& triggerCenter, const Vector3& point) const
{
    const float halfX = m_xWidth * 0.5f;
    const float halfZ = m_zWidth * 0.5f;

    return point.x >= triggerCenter.x - halfX &&
        point.x <= triggerCenter.x + halfX &&
        point.z >= triggerCenter.z - halfZ &&
        point.z <= triggerCenter.z + halfZ;
}

void TriggerArea::triggerSceneChange()
{
    if (m_sceneToLoad.empty())
    {
        return;
    }

    Scene::requestSceneChange(m_sceneToLoad.c_str());
}

IMPLEMENT_SCRIPT(TriggerArea)