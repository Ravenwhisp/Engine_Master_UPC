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

LightComponent::LightComponent(int id, GameObject* owner)
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
