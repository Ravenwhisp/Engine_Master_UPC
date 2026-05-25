#include "pch.h"
#include "TriggerArea.h"

IMPLEMENT_SCRIPT_FIELDS(TriggerArea,
    SERIALIZED_FLOAT(m_xWidth, "X Width", 0.0f, 1000.0f, 0.1f),
    SERIALIZED_FLOAT(m_zWidth, "Z Width", 0.0f, 1000.0f, 0.1f),
    SERIALIZED_STRING(m_sceneToLoad, "Scene To Load"),
    SERIALIZED_COMPONENT_REF(m_firstTarget, "First Target", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_secondTarget, "Second Target", ComponentType::TRANSFORM)
)

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
	Debug::log("Triggering scene change to: %s", m_sceneToLoad.c_str());
    SceneAPI::requestSceneChange(m_sceneToLoad.c_str());
}

IMPLEMENT_SCRIPT(TriggerArea)