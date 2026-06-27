#include "Globals.h"
#include "LightComponent.h"
#include "JsonArchive.h"
#include "GameObject.h"
#include "Transform.h"

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

    static uint32_t sanitizeShadowMapSize(uint32_t size)
    {
        if (size <= 1024)
        {
            return 1024;
        }

        if (size <= 2048)
        {
            return 2048;
        }

        if (size <= 4096)
        {
            return 4096;
        }

        return 8192;
    }

    static void sanitizeShadowSettings(LightShadowSettings& shadow)
    {
        shadow.shadowMapSize = sanitizeShadowMapSize(shadow.shadowMapSize);

        shadow.pcfRadius = std::clamp(
            shadow.pcfRadius,
            1u,
            2u);

        shadow.shadowBias = std::max(0.0f, shadow.shadowBias);

        shadow.shadowStrength = std::clamp(
            shadow.shadowStrength,
            0.0f,
            1.0f);
    }

}

LightComponent::LightComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::LIGHT, owner)
{
}

std::unique_ptr<Component> LightComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<LightComponent> newComponent = std::make_unique<LightComponent>(m_uuid, newOwner);

    newComponent->m_data = m_data;

	return newComponent;
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
    sanitizeShadowSettings(m_data.shadow);

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

    if (ImGui::CollapsingHeader("Shadow Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Checkbox("Cast Shadows", &m_data.shadow.castShadows))
        {
            lightChanged = true;
        }

        static const char* SHADOW_SIZE_NAMES[] =
        {
            "1024",
            "2048",
            "4096",
            "8192"
        };

        static constexpr uint32_t SHADOW_SIZES[] =
        {
            1024u,
            2048u,
            4096u,
            8192u
        };

        int shadowSizeIndex = 2; // Default 4096

        for (int i = 0; i < IM_ARRAYSIZE(SHADOW_SIZES); ++i)
        {
            if (m_data.shadow.shadowMapSize == SHADOW_SIZES[i])
            {
                shadowSizeIndex = i;
                break;
            }
        }

        if (ImGui::Combo("Shadow Map Size", &shadowSizeIndex, SHADOW_SIZE_NAMES, IM_ARRAYSIZE(SHADOW_SIZE_NAMES)))
        {
            m_data.shadow.shadowMapSize = SHADOW_SIZES[shadowSizeIndex];
            lightChanged = true;
        }

        if (ImGui::Checkbox("PCF Enabled", &m_data.shadow.pcfEnabled))
        {
            lightChanged = true;
        }

        if (m_data.shadow.pcfEnabled)
        {
            static const char* PCF_KERNEL_NAMES[] =
            {
                "3x3",
                "5x5"
            };

            int pcfKernelIndex = m_data.shadow.pcfRadius == 2 ? 1 : 0;

            if (ImGui::Combo("PCF Kernel", &pcfKernelIndex, PCF_KERNEL_NAMES, IM_ARRAYSIZE(PCF_KERNEL_NAMES)))
            {
                m_data.shadow.pcfRadius = pcfKernelIndex == 0 ? 1u : 2u;
                lightChanged = true;
            }
        }

        if (ImGui::DragFloat("Shadow Bias", &m_data.shadow.shadowBias, 0.0001f, 0.0f, 0.02f, "%.6f"))
        {
            lightChanged = true;
        }

        if (ImGui::DragFloat("Shadow Strength", &m_data.shadow.shadowStrength, 0.01f, 0.0f, 1.0f))
        {
            lightChanged = true;
        }

        if (m_data.type != LightType::DIRECTIONAL)
        {
            ImGui::TextDisabled("Shadow rendering for this light type is not implemented yet.");
        }
    }

    if (lightChanged) 
    {
        sanitize();
    }
}

void LightComponent::serialize(IArchive& archive)
{
    Component::serialize(archive);

    uint8_t lightType = static_cast<uint8_t>(m_data.type);
    archive.serialize(lightType, "LightType");
    if (archive.mode() == ArchiveMode::Input)
        m_data.type = static_cast<LightType>(lightType);

    archive.serialize(m_data.common.color, "Color");
    archive.serialize(m_data.common.intensity, "Intensity");

    archive.serialize(m_data.shadow.castShadows, "CastShadows");

    uint32_t shadowMapSize = static_cast<uint32_t>(m_data.shadow.shadowMapSize);
    archive.serialize(shadowMapSize, "ShadowMapSize");
    if (archive.mode() == ArchiveMode::Input)
        m_data.shadow.shadowMapSize = shadowMapSize;

    archive.serialize(m_data.shadow.pcfEnabled, "ShadowPcfEnabled");

    uint32_t shadowPcfRadius = static_cast<uint32_t>(m_data.shadow.pcfRadius);
    archive.serialize(shadowPcfRadius, "ShadowPcfRadius");
    if (archive.mode() == ArchiveMode::Input)
        m_data.shadow.pcfRadius = shadowPcfRadius;

    archive.serialize(m_data.shadow.shadowBias, "ShadowBias");
    archive.serialize(m_data.shadow.shadowStrength, "ShadowStrength");

    float radius = 0.0f;
    float innerAngle = 0.0f;
    float outerAngle = 0.0f;

    if (archive.mode() == ArchiveMode::Output)
    {
        switch (m_data.type)
        {
        case LightType::POINT:
            radius = m_data.parameters.point.radius;
            break;

        case LightType::SPOT:
            radius = m_data.parameters.spot.radius;
            innerAngle = m_data.parameters.spot.innerAngleDegrees;
            outerAngle = m_data.parameters.spot.outerAngleDegrees;
            break;

        default:
            break;
        }
    }

    archive.serialize(radius, "Radius");
    archive.serialize(innerAngle, "InnerAngleDegrees");
    archive.serialize(outerAngle, "OuterAngleDegrees");

    if (archive.mode() == ArchiveMode::Input)
    {
        switch (m_data.type)
        {
        case LightType::DIRECTIONAL:
            setTypeDirectional();
            break;

        case LightType::POINT:
            setTypePoint(radius);
            break;

        case LightType::SPOT:
            setTypeSpot(radius, innerAngle, outerAngle);
            break;
        }

        sanitize();
    }
}
void LightComponent::debugDraw()
{
    if ( !isActive() || !m_owner->GetActive())
    {
        return;
    }

    constexpr float DIRECTIONAL_ARROW_LENGTH = 2.0f;
    constexpr float DIRECTIONAL_ARROW_HEAD_LENGTH = 0.15f;
    constexpr float SPOT_DEBUG_MAX_ANGLE_DEGREES = 89.0f;

    const bool depthEnabled = false;

    const Transform* transform = m_owner->GetTransform();
    const Matrix& world = transform->getGlobalMatrix();
    const Vector3    position(world._41, world._42, world._43);

    Vector3 forward = transform->getForward();
    forward.Normalize();

    const Vector3 color = Vector3(1.0f, 1.0f, 0.0f);

    auto asFloat3 = [](const Vector3& v) { return &v.x; };

    switch (m_data.type)
    {
    case LightType::DIRECTIONAL:
    {
        const Vector3 endPosition = position + forward * DIRECTIONAL_ARROW_LENGTH;
        dd::arrow(asFloat3(position), asFloat3(endPosition), asFloat3(color), DIRECTIONAL_ARROW_HEAD_LENGTH, 0, depthEnabled);
        break;
    }
    case LightType::POINT:
    {
        dd::sphere(asFloat3(position), asFloat3(color), m_data.parameters.point.radius, 0, depthEnabled);
        break;
    }
    case LightType::SPOT:
    {
        const float length = m_data.parameters.spot.radius;

        float outerRadians = XMConvertToRadians(std::clamp(m_data.parameters.spot.outerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES));
        float innerRadians = XMConvertToRadians(std::clamp(m_data.parameters.spot.innerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES));

        Vector3 referenceAxis = (std::abs(forward.y) > 0.99f) ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);

        Vector3 x = referenceAxis.Cross(forward); x.Normalize();
        Vector3 y = forward.Cross(x);             y.Normalize();

        auto drawConeWire = [&](float angleRad)
            {
                float    baseCircleRadius = std::tan(angleRad) * length;
                Vector3  baseCenter = position + forward * length;
                Vector3  previousCirclePoint = baseCenter + y * baseCircleRadius;

                for (int angleDeg = 20; angleDeg <= 360; angleDeg += 20)
                {
                    float   s = std::sin(XMConvertToRadians((float)angleDeg));
                    float   c = std::cos(XMConvertToRadians((float)angleDeg));
                    Vector3 circlePoint = baseCenter + (x * s + y * c) * baseCircleRadius;

                    dd::line(asFloat3(previousCirclePoint), asFloat3(circlePoint), asFloat3(color), 0, depthEnabled);
                    dd::line(asFloat3(circlePoint), asFloat3(position), asFloat3(color), 0, depthEnabled);

                    previousCirclePoint = circlePoint;
                }
            };

        drawConeWire(outerRadians);
        drawConeWire(innerRadians);
        break;
    }
    default:
        break;
    }
}

