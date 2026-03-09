#include "Globals.h"
#include "CameraFollow.h"
#include "Application.h"
#include "SceneModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "TimeModule.h"
#include "ComponentType.h"

static const float PI = 3.1415926535897931f;

CameraFollow::CameraFollow(UID id, GameObject* gameObject)
    : Component(id, ComponentType::CAMERA_FOLLOW, gameObject)
{
}

std::unique_ptr<Component> CameraFollow::clone(GameObject* newOwner) const
{
    auto clonedComponent = std::make_unique<CameraFollow>(m_uuid, newOwner);

    clonedComponent->setActive(this->isActive());
    clonedComponent->m_firstTargetTransformUid = m_firstTargetTransformUid;
    clonedComponent->m_secondTargetTransformUid = m_secondTargetTransformUid;
    clonedComponent->m_transformOffset = m_transformOffset;
    clonedComponent->m_rotationOffset = m_rotationOffset;
    clonedComponent->m_zoomStartDistance = m_zoomStartDistance;
    clonedComponent->m_zoomEndDistance = m_zoomEndDistance;
    clonedComponent->m_maxExtraHeight = m_maxExtraHeight;
    clonedComponent->m_followSharpness = m_followSharpness;
    clonedComponent->m_zoomSharpness = m_zoomSharpness;

    clonedComponent->m_firstTargetTransform = nullptr;
    clonedComponent->m_secondTargetTransform = nullptr;
    clonedComponent->m_currentExtraHeight = 0.0f;

	return clonedComponent;
}

void CameraFollow::fixReferences(const std::unordered_map<UID, Component*>& referenceMap)
{
    m_firstTargetTransform = nullptr;
    m_secondTargetTransform = nullptr;
    m_currentExtraHeight = 0.0f;
    m_firstUpdateAfterResolve = true;

    if (m_firstTargetTransformUid != 0)
    {
        auto it = referenceMap.find(m_firstTargetTransformUid);
        if (it != referenceMap.end())
        {
            m_firstTargetTransform = static_cast<Transform*>(it->second);
        }
    }

    if (m_secondTargetTransformUid != 0)
    {
        auto it = referenceMap.find(m_secondTargetTransformUid);
        if (it != referenceMap.end())
        {
            m_secondTargetTransform = static_cast<Transform*>(it->second);
        }
    }

    if (!m_firstTargetTransform && m_secondTargetTransform)
    {
        m_firstTargetTransform = m_secondTargetTransform;
        m_firstTargetTransformUid = m_secondTargetTransformUid;
        m_secondTargetTransform = nullptr;
        m_secondTargetTransformUid = 0;
    }
}

bool CameraFollow::init()
{

    return true;
}

void CameraFollow::update()
{
    if (!m_firstTargetTransform) return;

    Transform* cameraTransform = m_owner->GetTransform();

    const float dt = app->getTimeModule()->deltaTime();

    const bool hasSecondTarget = (m_secondTargetTransform != nullptr);

    Vector3 followPoint = computeFollowPoint();

    float targetExtraHeight = 0.0f;
    if (hasSecondTarget)
    {
        const Vector3 p1 = m_firstTargetTransform->getPosition();
        const Vector3 p2 = m_secondTargetTransform->getPosition();
        targetExtraHeight = computeTargetExtraHeight(p1, p2);
    }

    m_currentExtraHeight = smoothExtraHeight(m_currentExtraHeight, targetExtraHeight, m_zoomSharpness, dt);

    const Vector3 desiredPos = computeDesiredCameraPosition(followPoint);

    if (m_firstUpdateAfterResolve)
    {
        cameraTransform->setPosition(desiredPos);
        cameraTransform->setRotationEuler(m_rotationOffset);
        m_firstUpdateAfterResolve = false;
        return;
    }

    const Vector3 smoothedCameraPosition = smoothCameraPosition(cameraTransform->getPosition(), desiredPos, m_followSharpness, dt);
    cameraTransform->setPosition(smoothedCameraPosition);

    cameraTransform->setRotationEuler(m_rotationOffset);
}

Vector3 CameraFollow::computeFollowPoint() const
{
    Vector3 followPoint;
    if (!m_secondTargetTransform) {
        followPoint = m_firstTargetTransform->getPosition();

        return followPoint;
    }

    const Vector3 p1 = m_firstTargetTransform->getPosition();
    const Vector3 p2 = m_secondTargetTransform->getPosition();
    followPoint = (p1 + p2) * 0.5f;

    return followPoint;
}

float CameraFollow::computeTargetExtraHeight(const Vector3& p1, const Vector3& p2) const 
{
    const float distance = (p2 - p1).Length();

    const float zoomRange = m_zoomEndDistance - m_zoomStartDistance;
    const float distancePastZoomStart = distance - m_zoomStartDistance;

    float normalizedZoomFactor = 0.0f;

    if (zoomRange > 0.0001f)
    {
        normalizedZoomFactor = distancePastZoomStart / zoomRange;
        if (normalizedZoomFactor < 0.0f)
        {
            normalizedZoomFactor = 0.0f;
        }
        if (normalizedZoomFactor > 1.0f)
        {
            normalizedZoomFactor = 1.0f;
        }
    }

    float targetExtraHeight = m_maxExtraHeight * normalizedZoomFactor;

    return targetExtraHeight;
}

float CameraFollow::smoothExtraHeight(float currentExtraHeight, float targetExtraHeight, float sharpness, float dt) const 
{
    if (sharpness <= 0.0f)
    {
        return targetExtraHeight;
    }

    const float zoomFraction = 1.0f - expf(-sharpness * dt);
    return lerpFloat(currentExtraHeight, targetExtraHeight, zoomFraction);
}

Vector3 CameraFollow::computeDesiredCameraPosition(const Vector3& followPoint) const
{
    Vector3 desiredPos = followPoint;

    // XZ follows midpoint
    desiredPos.x += m_transformOffset.x;
    desiredPos.z += m_transformOffset.z;

    // Y follows the highest target
    float highestTargetY = m_firstTargetTransform->getPosition().y;

    if (m_secondTargetTransform)
    {
        const float secondTargetY = m_secondTargetTransform->getPosition().y;
        if (secondTargetY > highestTargetY)
        {
            highestTargetY = secondTargetY;
        }
    }

    desiredPos.y = highestTargetY + m_transformOffset.y + m_currentExtraHeight;

    return desiredPos;
}

Vector3 CameraFollow::smoothCameraPosition(const Vector3& currentPos, const Vector3& targetPos, float sharpness, float dt) const {
    if (sharpness <= 0.0f)
    {
        return targetPos;
    }

    const float followFraction = 1.0f - expf(-sharpness * dt);
    return lerpVector(currentPos, targetPos, followFraction);
}

Vector3 CameraFollow::lerpVector(const Vector3& start, const Vector3& end, float alpha) const
{
    return start + (end - start) * alpha;
}

float CameraFollow::lerpFloat(float start, float end, float alpha) const
{
    return start + (end - start) * alpha;
}

void CameraFollow::drawUi()
{
    ImGui::Text("Camera Follow");

    ImGui::SeparatorText("First Target");

    if (m_firstTargetTransform)
    {
        ImGui::TextColored(
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
            "%s",
            m_firstTargetTransform->getOwner()->GetName().c_str()
        );
    }
    else
    {
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
            "Drag a PLAYER_WALK here"
        );
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
        {
            GameObject* droppedObject = *(GameObject**)payload->Data;
            GameObject* sceneObject = app->getSceneModule()->findGameObjectByUID(droppedObject->GetID());

            if (sceneObject && sceneObject->GetComponent(ComponentType::PLAYER_WALK))
            {
                m_firstTargetTransform = sceneObject->GetTransform();
                m_firstTargetTransformUid = m_firstTargetTransform ? m_firstTargetTransform->getID() : 0;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear###ClearFirstTargetButton"))
    {
        m_firstTargetTransform = nullptr;
        m_firstTargetTransformUid = 0;
    }

    ImGui::SeparatorText("Second Target");

    if (m_secondTargetTransform)
    {
        ImGui::TextColored(
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
            "%s",
            m_secondTargetTransform->getOwner()->GetName().c_str()
        );
    }
    else
    {
        ImGui::TextColored(
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
            "Drag a PLAYER_WALK here"
        );
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
        {
            GameObject* droppedObject = *(GameObject**)payload->Data;
            GameObject* sceneObject = app->getSceneModule()->findGameObjectByUID(droppedObject->GetID());

            if (sceneObject && sceneObject->GetComponent(ComponentType::PLAYER_WALK))
            {
                m_secondTargetTransform = sceneObject->GetTransform();
                m_secondTargetTransformUid = m_secondTargetTransform ? m_secondTargetTransform->getID() : 0;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear###ClearSecondTargetButton"))
    {
        m_secondTargetTransform = nullptr;
        m_secondTargetTransformUid = 0;
    }

    ImGui::SeparatorText("Camera Transform");

    ImGui::DragFloat3("World Offset", &m_transformOffset.x, 0.05f);
    ImGui::DragFloat3("Fixed Rotation", &m_rotationOffset.x, 0.25f);

    ImGui::SeparatorText("Smoothing");

    ImGui::DragFloat("Follow Sharpness", &m_followSharpness, 0.1f, 0.0f, 50.0f);
    ImGui::DragFloat("Zoom Sharpness", &m_zoomSharpness, 0.1f, 0.0f, 50.0f);

    ImGui::SeparatorText("Zoom by Distance (2 targets)");

    ImGui::DragFloat("Zoom Start Distance", &m_zoomStartDistance, 0.05f, 0.0f, 1000.0f);
    ImGui::DragFloat("Zoom End Distance", &m_zoomEndDistance, 0.05f, 0.0f, 1000.0f);
    ImGui::DragFloat("Max Extra Height", &m_maxExtraHeight, 0.05f, 0.0f, 1000.0f);
}

rapidjson::Value CameraFollow::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::CAMERA_FOLLOW), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("FirstTargetUID", (uint64_t)m_firstTargetTransformUid, domTree.GetAllocator());
    componentInfo.AddMember("SecondTargetUID", (uint64_t)m_secondTargetTransformUid, domTree.GetAllocator());

    {
        rapidjson::Value offset(rapidjson::kArrayType);
        offset.PushBack(m_transformOffset.x, domTree.GetAllocator());
        offset.PushBack(m_transformOffset.y, domTree.GetAllocator());
        offset.PushBack(m_transformOffset.z, domTree.GetAllocator());
        componentInfo.AddMember("WorldOffset", offset, domTree.GetAllocator());
    }

    {
        rapidjson::Value rot(rapidjson::kArrayType);
        rot.PushBack(m_rotationOffset.x, domTree.GetAllocator());
        rot.PushBack(m_rotationOffset.y, domTree.GetAllocator());
        rot.PushBack(m_rotationOffset.z, domTree.GetAllocator());
        componentInfo.AddMember("FixedRotation", rot, domTree.GetAllocator());
    }

    componentInfo.AddMember("FollowSharpness", m_followSharpness, domTree.GetAllocator());
    componentInfo.AddMember("ZoomSharpness", m_zoomSharpness, domTree.GetAllocator());

    componentInfo.AddMember("ZoomStartDistance", m_zoomStartDistance, domTree.GetAllocator());
    componentInfo.AddMember("ZoomEndDistance", m_zoomEndDistance, domTree.GetAllocator());
    componentInfo.AddMember("MaxExtraHeight", m_maxExtraHeight, domTree.GetAllocator());

    return componentInfo;
}

rapidjson::Value CameraFollow::getNewJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", GenerateUID(), domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::CAMERA_FOLLOW), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("FirstTargetUID", m_firstTargetUid, domTree.GetAllocator());
    componentInfo.AddMember("SecondTargetUID", m_secondTargetUid, domTree.GetAllocator());

    {
        rapidjson::Value offset(rapidjson::kArrayType);
        offset.PushBack(m_transformOffset.x, domTree.GetAllocator());
        offset.PushBack(m_transformOffset.y, domTree.GetAllocator());
        offset.PushBack(m_transformOffset.z, domTree.GetAllocator());
        componentInfo.AddMember("WorldOffset", offset, domTree.GetAllocator());
    }

    {
        rapidjson::Value rot(rapidjson::kArrayType);
        rot.PushBack(m_rotationOffset.x, domTree.GetAllocator());
        rot.PushBack(m_rotationOffset.y, domTree.GetAllocator());
        rot.PushBack(m_rotationOffset.z, domTree.GetAllocator());
        componentInfo.AddMember("FixedRotation", rot, domTree.GetAllocator());
    }

    componentInfo.AddMember("FollowSharpness", m_followSharpness, domTree.GetAllocator());
    componentInfo.AddMember("ZoomSharpness", m_zoomSharpness, domTree.GetAllocator());

    componentInfo.AddMember("ZoomStartDistance", m_zoomStartDistance, domTree.GetAllocator());
    componentInfo.AddMember("ZoomEndDistance", m_zoomEndDistance, domTree.GetAllocator());
    componentInfo.AddMember("MaxExtraHeight", m_maxExtraHeight, domTree.GetAllocator());

    return componentInfo;
}

bool CameraFollow::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("FirstTargetUID"))
    {
        m_firstTargetTransformUid = (UID)componentInfo["FirstTargetUID"].GetUint64();
    }
    if (componentInfo.HasMember("SecondTargetUID"))
    {
        m_secondTargetTransformUid = (UID)componentInfo["SecondTargetUID"].GetUint64();
    }

    if (componentInfo.HasMember("WorldOffset"))
    {
        const auto& worldOffsetArray = componentInfo["WorldOffset"];
        m_transformOffset.x = worldOffsetArray[0].GetFloat();
        m_transformOffset.y = worldOffsetArray[1].GetFloat();
        m_transformOffset.z = worldOffsetArray[2].GetFloat();
    }

    if (componentInfo.HasMember("FixedRotation"))
    {
        const auto& fixedRotationArray = componentInfo["FixedRotation"];
        m_rotationOffset.x = fixedRotationArray[0].GetFloat();
        m_rotationOffset.y = fixedRotationArray[1].GetFloat();
        m_rotationOffset.z = fixedRotationArray[2].GetFloat();
    }

    if (componentInfo.HasMember("FollowSharpness"))
    {
        m_followSharpness = componentInfo["FollowSharpness"].GetFloat();
    }
    if (componentInfo.HasMember("ZoomSharpness"))
    {
        m_zoomSharpness = componentInfo["ZoomSharpness"].GetFloat();
    }

    if (componentInfo.HasMember("ZoomStartDistance"))
    {
        m_zoomStartDistance = componentInfo["ZoomStartDistance"].GetFloat();
    }

    if (componentInfo.HasMember("ZoomEndDistance"))
    {
        m_zoomEndDistance = componentInfo["ZoomEndDistance"].GetFloat();
    }

    if (componentInfo.HasMember("MaxExtraHeight"))
    {
        m_maxExtraHeight = componentInfo["MaxExtraHeight"].GetFloat();
    }

    m_firstTargetTransform = nullptr;
    m_secondTargetTransform = nullptr;
    m_currentExtraHeight = 0.0f;

    return true;
}