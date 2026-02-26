#include "Globals.h"
#include "LightComponent.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float SPOT_MAX_ANGLE_DEGREES = 89.9f;
    constexpr float SPOT_MIN_ANGLE_DELTA_DEGREES = 0.001f;

    static void sanitizeSpotAngles(float& innerAngleDegrees, float& outerAngleDegrees)
    {
        innerAngleDegrees = std::clamp(innerAngleDegrees, 0.0f, SPOT_MAX_ANGLE_DEGREES);
        outerAngleDegrees = std::clamp(outerAngleDegrees, 0.0f, SPOT_MAX_ANGLE_DEGREES);

        if (innerAngleDegrees > outerAngleDegrees)
        {
            std::swap(innerAngleDegrees, outerAngleDegrees);
        }

        if (std::abs(innerAngleDegrees - outerAngleDegrees) < SPOT_MIN_ANGLE_DELTA_DEGREES)
        {
            outerAngleDegrees = std::min(SPOT_MAX_ANGLE_DEGREES, innerAngleDegrees + SPOT_MIN_ANGLE_DELTA_DEGREES);
        }
    }
}

LightComponent::LightComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::LIGHT, owner)
{
}

void LightComponent::setTypeDirectional()
{
    m_data.type = LightType::DIRECTIONAL;
    m_data.parameters = LightParameters::makeDirectional();
    sanitize();
}

void LightComponent::setTypePoint(float radius)
{
    m_data.type = LightType::POINT;
    m_data.parameters = LightParameters::makePoint(radius);
    sanitize();
}

void LightComponent::setTypeSpot(float radius, float innerAngleDegrees, float outerAngleDegrees)
{
    m_data.type = LightType::SPOT;
    m_data.parameters = LightParameters::makeSpot(radius, innerAngleDegrees, outerAngleDegrees);
    sanitize();
}

void LightComponent::sanitize()
{
    m_data.common.intensity = std::max(0.0f, m_data.common.intensity);

    if (m_data.type == LightType::POINT)
    {
        m_data.parameters.point.radius = std::max(0.0f, m_data.parameters.point.radius);
        return;
    }

    if (m_data.type == LightType::SPOT)
    {
        m_data.parameters.spot.radius = std::max(0.0f, m_data.parameters.spot.radius);
        sanitizeSpotAngles(
            m_data.parameters.spot.innerAngleDegrees,
            m_data.parameters.spot.outerAngleDegrees);
        return;
    }
}

void LightComponent::drawUi()
{
    bool lightChanged = false;

    {
        static const char* TYPE_NAMES[(int)LightType::COUNT] = { "Directional", "Point", "Spot" };

        int typeIndex = static_cast<int>(m_data.type);

        if (ImGui::Combo("Type", &typeIndex, TYPE_NAMES, (int)LightType::COUNT))
        {
            const LightType newType = static_cast<LightType>(typeIndex);

            if (newType == LightType::DIRECTIONAL) 
            {
                setTypeDirectional();
            }
            else if (newType == LightType::POINT) 
            {
                setTypePoint(m_data.parameters.point.radius);
            }
            else if (newType == LightType::SPOT) 
            {
                setTypeSpot(m_data.parameters.spot.radius, m_data.parameters.spot.innerAngleDegrees, m_data.parameters.spot.outerAngleDegrees);
            }

            lightChanged = true;
        }
    }

    ImGui::Separator();

    float rgb[3] = { m_data.common.color.x, m_data.common.color.y, m_data.common.color.z };
    if (ImGui::ColorEdit3("Color", rgb))
    {
        m_data.common.color = Vector3(rgb[0], rgb[1], rgb[2]);
        lightChanged = true;
    }

    if (ImGui::DragFloat("Intensity", &m_data.common.intensity, 0.1f, 0.0f, 500.0f))
    {
        lightChanged = true;
    }

    ImGui::Separator();

    switch (m_data.type)
    {
    case LightType::DIRECTIONAL:
        ImGui::Text("Directional");
        break;

    case LightType::POINT:
        ImGui::Text("Point");
        if (ImGui::DragFloat("Radius", &m_data.parameters.point.radius, 0.1f, 0.0f, 200.0f))
        {
            lightChanged = true;
        }
        break;

    case LightType::SPOT:
        ImGui::Text("Spot");
        if (ImGui::DragFloat("Radius##Spot", &m_data.parameters.spot.radius, 0.1f, 0.0f, 200.0f))
        {
            lightChanged = true;
        }

        if (ImGui::DragFloat("Inner Angle##Spot", &m_data.parameters.spot.innerAngleDegrees, 0.1f, 0.0f, 179.0f)) 
        {
            lightChanged = true;
        }

        if (ImGui::DragFloat("Outer Angle##Spot", &m_data.parameters.spot.outerAngleDegrees, 0.1f, 0.0f, 179.0f)) 
        {
            lightChanged = true;
        }
        break;

    default:
        ImGui::TextDisabled("Unknown light type.");
        break;
    }

    ImGui::Separator();
    ImGui::Text("Debug");

    if (ImGui::Checkbox("Draw Debug", &m_debugDrawEnabled));

    if (m_debugDrawEnabled)
    {
        ImGui::Checkbox("Depth Test", &m_debugDrawDepthEnabled);
    }

    if (lightChanged) {
        sanitize();
    }
}

rapidjson::Value LightComponent::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", unsigned int(ComponentType::LIGHT), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("LightType", unsigned int(m_data.type), domTree.GetAllocator());
    {
        rapidjson::Value colorData(rapidjson::kArrayType);

        colorData.PushBack(m_data.common.color.x, domTree.GetAllocator());
        colorData.PushBack(m_data.common.color.y, domTree.GetAllocator());
        colorData.PushBack(m_data.common.color.z, domTree.GetAllocator());

        componentInfo.AddMember("Color", colorData, domTree.GetAllocator());
    }
    componentInfo.AddMember("Intensity", m_data.common.intensity, domTree.GetAllocator());
    
    // Not common parameters (depending on light type)
    switch (m_data.type)
    {

    case LightType::DIRECTIONAL : 

        break;

    case LightType::POINT:
        
        componentInfo.AddMember("Radius", m_data.parameters.point.radius, domTree.GetAllocator());
        break;
    
    case LightType::SPOT:

        componentInfo.AddMember("Radius", m_data.parameters.spot.radius, domTree.GetAllocator());
        componentInfo.AddMember("InnerAngleDegrees", m_data.parameters.spot.innerAngleDegrees, domTree.GetAllocator());
        componentInfo.AddMember("OuterAngleDegrees", m_data.parameters.spot.outerAngleDegrees, domTree.GetAllocator());
    }

    return componentInfo;
}
    
bool LightComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("Intensity")) {
        m_data.common.intensity = componentInfo["Intensity"].GetFloat();
    }

    if (componentInfo.HasMember("Color"))
    {
        const auto& color = componentInfo["Color"].GetArray();
        m_data.common.color = Vector3(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat());
    }

    if (componentInfo.HasMember("LightType"))
    {
        int typeInt = componentInfo["LightType"].GetInt();
        LightType type = static_cast<LightType>(typeInt);

        if (type == LightType::DIRECTIONAL)
        {
            setTypeDirectional();
        }
        else if (type == LightType::POINT)
        {
            float radius = m_data.parameters.point.radius;
            if (componentInfo.HasMember("Radius"))
                radius = componentInfo["Radius"].GetFloat();

            setTypePoint(radius);
        }
        else if (type == LightType::SPOT)
        {
            float radius = m_data.parameters.spot.radius;
            float innerA = m_data.parameters.spot.innerAngleDegrees;
            float outerA = m_data.parameters.spot.outerAngleDegrees;

            if (componentInfo.HasMember("Radius"))
                radius = componentInfo["Radius"].GetFloat();

            if (componentInfo.HasMember("InnerAngleDegrees"))
                innerA = componentInfo["InnerAngleDegrees"].GetFloat();

            if (componentInfo.HasMember("OuterAngleDegrees"))
                outerA = componentInfo["OuterAngleDegrees"].GetFloat();

            setTypeSpot(radius, innerA, outerA);
        }
    }

    sanitize();
    return true;
}

