#include "pch.h"
#include "CrystalVisuals.h"

IMPLEMENT_SCRIPT_FIELDS(CrystalVisuals,
    SERIALIZED_COMPONENT_REF(m_rotationPivot, "Object to Rotate (cristal)", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_inactiveModel, "Inactive Model", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_activeModel, "Active Model", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_rotationSpeed, "Rotation Speed", -360.0f, 360.0f, 1.0f)
)

CrystalVisuals::CrystalVisuals(GameObject* owner)
    : Script(owner)
{
}

void CrystalVisuals::Start()
{
    resolveReferences();
    updateModelVisibility();
}

void CrystalVisuals::Update()
{
    updateRotation(Time::getDeltaTime());
}

void CrystalVisuals::setActivated(bool activated)
{
    Debug::log("[CrystalVisuals] setActivated called with: %s", activated ? "true" : "false");

    m_activated = activated;
    updateModelVisibility();
}

void CrystalVisuals::resolveReferences()
{
    m_rotationPivotTransform = m_rotationPivot.getReferencedComponent();
    m_inactiveModelTransform = m_inactiveModel.getReferencedComponent();
    m_activeModelTransform = m_activeModel.getReferencedComponent();

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

    if (ownerTransform == nullptr)
    {
        Debug::warn("[CrystalVisual] '%s' has no Transform.", GameObjectAPI::getName(getOwner()));
        return;
    }

    if (m_rotationPivotTransform == nullptr)
    {
        m_rotationPivotTransform = TransformAPI::findChildByName(ownerTransform, "Crystal Pivot");
    }

    if (m_rotationPivotTransform != nullptr)
    {
        if (m_inactiveModelTransform == nullptr)
        {
            m_inactiveModelTransform = TransformAPI::findChildByName(m_rotationPivotTransform, "Inactive");
        }

        if (m_activeModelTransform == nullptr)
        {
            m_activeModelTransform = TransformAPI::findChildByName(m_rotationPivotTransform, "Active");
        }
    }
}

void CrystalVisuals::updateRotation(float deltaTime)
{
    if (m_rotationPivotTransform == nullptr)
    {
        return;
    }

    Vector3 rotation = TransformAPI::getEulerDegrees(m_rotationPivotTransform);

    rotation.y += m_rotationSpeed * deltaTime;

    if (rotation.y >= 360.0f)
    {
        rotation.y -= 360.0f;
    }
    else if (rotation.y <= -360.0f)
    {
        rotation.y += 360.0f;
    }

    TransformAPI::setRotationEuler(m_rotationPivotTransform, rotation);
}

void CrystalVisuals::updateModelVisibility()
{
    Debug::log("[CrystalVisuals] Updating visibility. Activated: %s", m_activated ? "true" : "false");

    if (m_inactiveModelTransform != nullptr)
    {
        GameObject* inactiveObject = ComponentAPI::getOwner(m_inactiveModelTransform);
        Debug::log("[CrystalVisuals] Inactive object: %s -> active: %s", GameObjectAPI::getName(inactiveObject), !m_activated ? "true" : "false");
        GameObjectAPI::setActive(inactiveObject, !m_activated);
    }
    else
    {
        Debug::warn("[CrystalVisuals] Inactive model Transform is null.");
    }

    if (m_activeModelTransform != nullptr)
    {
        GameObject* activeObject = ComponentAPI::getOwner(m_activeModelTransform);
        Debug::log("[CrystalVisuals] Active object: %s -> active: %s", GameObjectAPI::getName(activeObject), m_activated ? "true" : "false");
        GameObjectAPI::setActive(activeObject, m_activated);
    }
    else
    {
        Debug::warn("[CrystalVisuals] Active model Transform is null.");
    }
}

IMPLEMENT_SCRIPT(CrystalVisuals)