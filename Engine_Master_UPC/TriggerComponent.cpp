#include "Globals.h"
#include "TriggerComponent.h"

#include "GameObject.h"
#include "Transform.h"

#include "Application.h"
#include "ModuleTrigger.h"
#include "MeshRenderer.h"

#include <array>

CEREAL_REGISTER_TYPE(TriggerComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, TriggerComponent)

TriggerComponent::TriggerComponent(UID id, GameObject* gameObject)
    : Component(id, ComponentType::TRIGGER, gameObject)
{
}

bool TriggerComponent::init()
{
    if (m_setDefaultBoundsOnInit)
    {
        setDefaultBoundsFromModel();
        m_setDefaultBoundsOnInit = false;
    }

    app->getModuleTrigger()->registerTrigger(this);
    return true;
}

bool TriggerComponent::cleanUp()
{
    app->getModuleTrigger()->unregisterTrigger(this);
    return true;
}

void TriggerComponent::drawUi()
{
    int debugMode = static_cast<int>(m_debugDrawMode);

    ImGui::Text("Debug Draw");

    if (ImGui::RadioButton("Rotated Trigger Box", debugMode == static_cast<int>(TriggerDebugDrawMode::RotatedBox)))
    {
        m_debugDrawMode = TriggerDebugDrawMode::RotatedBox;
    }

    if (ImGui::RadioButton("Detection AABB", debugMode == static_cast<int>(TriggerDebugDrawMode::DetectionAABB)))
    {
        m_debugDrawMode = TriggerDebugDrawMode::DetectionAABB;
    }

    ImGui::Separator();

    if (ImGui::DragFloat3("Center", &m_center.x, 0.05f))
    {
        m_boundsDirty = true;
    }

    if (ImGui::DragFloat3("Size", &m_size.x, 0.05f, 0.0f))
    {
        m_size.x = std::max(0.0f, m_size.x);
        m_size.y = std::max(0.0f, m_size.y);
        m_size.z = std::max(0.0f, m_size.z);

        m_boundsDirty = true;
    }

    ImGui::Separator();

    if (ImGui::Button("Reset Default Model Bounds"))
    {
        setDefaultBoundsFromModel();
    }
}

void TriggerComponent::debugDraw()
{
    if (!isActive())
    {
        return;
    }

    switch (m_debugDrawMode)
    {
    case TriggerDebugDrawMode::RotatedBox:
        getWorldBox().render();
        break;

    case TriggerDebugDrawMode::DetectionAABB:
        getWorldAABB().render();
        break;

    default:
        getWorldBox().render();
        break;
    }
}

void TriggerComponent::setCenter(const Vector3& center)
{
    m_center = center;
    m_boundsDirty = true;
}

void TriggerComponent::setSize(const Vector3& size)
{
    m_size.x = std::max(0.0f, size.x);
    m_size.y = std::max(0.0f, size.y);
    m_size.z = std::max(0.0f, size.z);

    m_boundsDirty = true;
}

Engine::BoundingBox& TriggerComponent::getWorldBox()
{
    if (m_boundsDirty)
    {
        recalculateWorldBounds();
    }

    return m_worldBox;
}

Engine::BoundingBox& TriggerComponent::getWorldAABB()
{
    if (m_boundsDirty)
    {
        recalculateWorldBounds();
    }

    return m_worldAABB;
}

void TriggerComponent::recalculateWorldBounds()
{
    if (!m_owner || !getTransform() || !isValidSize())
    {
        const Vector3 zero = Vector3::Zero;

        const Vector3 points[8] =
        {
            zero, zero, zero, zero,
            zero, zero, zero, zero
        };

        m_worldBox = Engine::BoundingBox(zero, zero, points);
        m_worldAABB = Engine::BoundingBox(zero, zero, points);
        m_boundsDirty = false;
        return;
    }

    const Vector3 halfSize = m_size * 0.5f;

    const Vector3 localMin = m_center - halfSize;
    const Vector3 localMax = m_center + halfSize;

    const Vector3 localCorners[8] =
    {
        Vector3(localMin.x, localMin.y, localMin.z),
        Vector3(localMax.x, localMin.y, localMin.z),
        Vector3(localMax.x, localMax.y, localMin.z),
        Vector3(localMin.x, localMax.y, localMin.z),

        Vector3(localMin.x, localMin.y, localMax.z),
        Vector3(localMax.x, localMin.y, localMax.z),
        Vector3(localMax.x, localMax.y, localMax.z),
        Vector3(localMin.x, localMax.y, localMax.z)
    };

    const Matrix& worldMatrix = getTransform()->getGlobalMatrix();

    Vector3 worldCorners[8];

    Vector3 worldMin(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3 worldMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (int i = 0; i < 8; ++i)
    {
        worldCorners[i] = Vector3::Transform(localCorners[i], worldMatrix);

        worldMin.x = std::min(worldMin.x, worldCorners[i].x);
        worldMin.y = std::min(worldMin.y, worldCorners[i].y);
        worldMin.z = std::min(worldMin.z, worldCorners[i].z);

        worldMax.x = std::max(worldMax.x, worldCorners[i].x);
        worldMax.y = std::max(worldMax.y, worldCorners[i].y);
        worldMax.z = std::max(worldMax.z, worldCorners[i].z);
    }

    m_worldBox = Engine::BoundingBox(worldMin, worldMax, worldCorners);

    const Vector3 aabbPoints[8] =
    {
        Vector3(worldMin.x, worldMin.y, worldMin.z),
        Vector3(worldMax.x, worldMin.y, worldMin.z),
        Vector3(worldMax.x, worldMax.y, worldMin.z),
        Vector3(worldMin.x, worldMax.y, worldMin.z),

        Vector3(worldMin.x, worldMin.y, worldMax.z),
        Vector3(worldMax.x, worldMin.y, worldMax.z),
        Vector3(worldMax.x, worldMax.y, worldMax.z),
        Vector3(worldMin.x, worldMax.y, worldMax.z)
    };

    m_worldAABB = Engine::BoundingBox(worldMin, worldMax, aabbPoints);

    m_boundsDirty = false;
}

bool TriggerComponent::isValidSize() const
{
    return m_size.x > 0.0f && m_size.y > 0.0f && m_size.z > 0.0f;
}

bool TriggerComponent::setDefaultBoundsFromModel()
{
    if (!m_owner || !m_owner->GetTransform())
    {
        return false;
    }

    Vector3 boundsMin(FLT_MAX, FLT_MAX, FLT_MAX);
    Vector3 boundsMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool hasBounds = false;

    const Matrix triggerWorldInverse = m_owner->GetTransform()->getGlobalMatrix().Invert();

    includeHierarchyModelBounds(m_owner, triggerWorldInverse, boundsMin, boundsMax, hasBounds);

    if (!hasBounds)
    {
        return false;
    }

    const Vector3 size = boundsMax - boundsMin;

    if (size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f)
    {
        return false;
    }

    setCenter((boundsMin + boundsMax) * 0.5f);
    setSize(size);

    return true;
}

void TriggerComponent::includeHierarchyModelBounds(GameObject* object, const Matrix& triggerWorldInverse, Vector3& boundsMin, Vector3& boundsMax, bool& hasBounds)
{
    if (!object || !object->GetTransform())
    {
        return;
    }

    MeshRenderer* meshRenderer = object->GetComponentAs<MeshRenderer>(ComponentType::MODEL);

    if (meshRenderer && meshRenderer->hasMesh())
    {
        includeMeshRendererBounds(meshRenderer, triggerWorldInverse, boundsMin, boundsMax, hasBounds);
    }

    for (GameObject* child : object->GetTransform()->getAllChildren())
    {
        includeHierarchyModelBounds(child, triggerWorldInverse, boundsMin, boundsMax, hasBounds);
    }
}

void TriggerComponent::includeMeshRendererBounds(MeshRenderer* meshRenderer, const Matrix& triggerWorldInverse, Vector3& boundsMin, Vector3& boundsMax, bool& hasBounds)
{
    if (!meshRenderer || !meshRenderer->hasMesh())
    {
        return;
    }

    GameObject* meshOwner = meshRenderer->getOwner();

    if (!meshOwner || !meshOwner->GetTransform())
    {
        return;
    }

    Engine::BoundingBox& meshBounds = meshRenderer->getBoundingBox();

    const Vector3& meshMin = meshBounds.getMin();
    const Vector3& meshMax = meshBounds.getMax();

    const Vector3 meshLocalCorners[8] =
    {
        Vector3(meshMin.x, meshMin.y, meshMin.z),
        Vector3(meshMax.x, meshMin.y, meshMin.z),
        Vector3(meshMax.x, meshMax.y, meshMin.z),
        Vector3(meshMin.x, meshMax.y, meshMin.z),

        Vector3(meshMin.x, meshMin.y, meshMax.z),
        Vector3(meshMax.x, meshMin.y, meshMax.z),
        Vector3(meshMax.x, meshMax.y, meshMax.z),
        Vector3(meshMin.x, meshMax.y, meshMax.z)
    };

    const Matrix& meshWorldMatrix = meshOwner->GetTransform()->getGlobalMatrix();

    for (int i = 0; i < 8; ++i)
    {
        const Vector3 worldCorner = Vector3::Transform(meshLocalCorners[i], meshWorldMatrix);
        const Vector3 triggerLocalCorner = Vector3::Transform(worldCorner, triggerWorldInverse);

        boundsMin.x = std::min(boundsMin.x, triggerLocalCorner.x);
        boundsMin.y = std::min(boundsMin.y, triggerLocalCorner.y);
        boundsMin.z = std::min(boundsMin.z, triggerLocalCorner.z);

        boundsMax.x = std::max(boundsMax.x, triggerLocalCorner.x);
        boundsMax.y = std::max(boundsMax.y, triggerLocalCorner.y);
        boundsMax.z = std::max(boundsMax.z, triggerLocalCorner.z);

        hasBounds = true;
    }
}

rapidjson::Value TriggerComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& allocator = domTree.GetAllocator();

    componentInfo.AddMember("UID", m_uuid, allocator);
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::TRIGGER), allocator);
    componentInfo.AddMember("Active", isActive(), allocator);
    componentInfo.AddMember("Shape", static_cast<int>(m_shape), allocator);

    {
        rapidjson::Value centerData(rapidjson::kArrayType);

        centerData.PushBack(m_center.x, allocator);
        centerData.PushBack(m_center.y, allocator);
        centerData.PushBack(m_center.z, allocator);

        componentInfo.AddMember("Center", centerData, allocator);
    }

    {
        rapidjson::Value sizeData(rapidjson::kArrayType);

        sizeData.PushBack(m_size.x, allocator);
        sizeData.PushBack(m_size.y, allocator);
        sizeData.PushBack(m_size.z, allocator);

        componentInfo.AddMember("Size", sizeData, allocator);
    }

    return componentInfo;
}

bool TriggerComponent::deserializeJSON(const rapidjson::Value& componentValue)
{
    if (componentValue.HasMember("Active") && componentValue["Active"].IsBool())
    {
        setActive(componentValue["Active"].GetBool());
    }

    if (componentValue.HasMember("Shape") && componentValue["Shape"].IsInt())
    {
        m_shape = static_cast<TriggerShape>(componentValue["Shape"].GetInt());
    }

    if (componentValue.HasMember("Center") && componentValue["Center"].IsArray())
    {
        const rapidjson::Value& centerData = componentValue["Center"];

        if (centerData.Size() == 3 &&
            centerData[0].IsNumber() &&
            centerData[1].IsNumber() &&
            centerData[2].IsNumber())
        {
            m_center.x = centerData[0].GetFloat();
            m_center.y = centerData[1].GetFloat();
            m_center.z = centerData[2].GetFloat();
        }
    }

    if (componentValue.HasMember("Size") && componentValue["Size"].IsArray())
    {
        const rapidjson::Value& sizeData = componentValue["Size"];

        if (sizeData.Size() == 3 &&
            sizeData[0].IsNumber() &&
            sizeData[1].IsNumber() &&
            sizeData[2].IsNumber())
        {
            m_size.x = std::max(0.0f, sizeData[0].GetFloat());
            m_size.y = std::max(0.0f, sizeData[1].GetFloat());
            m_size.z = std::max(0.0f, sizeData[2].GetFloat());
        }
    }

    m_setDefaultBoundsOnInit = false;
    m_boundsDirty = true;
    return true;
}

std::unique_ptr<Component> TriggerComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<TriggerComponent> clonedComponent = std::make_unique<TriggerComponent>(m_uuid, newOwner);

    clonedComponent->m_shape = m_shape;
    clonedComponent->m_center = m_center;
    clonedComponent->m_size = m_size;
    clonedComponent->m_debugDrawMode = m_debugDrawMode;
    clonedComponent->m_setDefaultBoundsOnInit = false;
    clonedComponent->setActive(isActive());
    clonedComponent->m_boundsDirty = true;

    return clonedComponent;
}