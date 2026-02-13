#include "Globals.h"
#include "LightDebugDraw.h"

#include "GameObject.h"
#include "Transform.h"
#include "LightComponent.h"
#include "ComponentType.h"

#include <algorithm>

namespace
{
    constexpr float DIRECTIONAL_ARROW_LENGTH = 2.0f;
    constexpr float DIRECTIONAL_ARROW_HEAD_LENGTH = 0.15f;
    constexpr float SPOT_DEBUG_MAX_ANGLE_DEGREES = 89.0f;

    static inline const float* asFloat3(const Vector3& v)
    {
        return &v.x;
    }

    static void drawLight(const GameObject& gameObject, bool depthEnabled)
    {
        if (!gameObject.GetActive())
        {
            return;
        }

        const LightComponent* lightComponent = gameObject.GetComponentAs<LightComponent>(ComponentType::LIGHT);
        if (lightComponent == nullptr)
        {
            return;
        }

        const LightData& lightData = lightComponent->getData();
        if (!lightData.common.enabled)
        {
            return;
        }

        const Transform* transform = gameObject.GetTransform();
        if (transform == nullptr)
        {
            return;
        }

        const Vector3 position = transform->getPosition();

        Vector3 forward = transform->getForward();
        forward.Normalize();

        const Vector3 color = Vector3(1.0f, 1.0f, 0.0f);

        switch (lightData.type)
        {
        case LightType::DIRECTIONAL:
        {
            const Vector3 endPosition = position + forward * DIRECTIONAL_ARROW_LENGTH;
            dd::arrow(asFloat3(position), asFloat3(endPosition), asFloat3(color), DIRECTIONAL_ARROW_HEAD_LENGTH, 0, depthEnabled);
            break;
        }
        case LightType::POINT:
        {
            dd::sphere(asFloat3(position), asFloat3(color), lightData.parameters.point.radius, 0, depthEnabled);
            break;
        }
        case LightType::SPOT:
        {
            const float length = lightData.parameters.spot.radius;

            const float outerDegrees = std::clamp(lightData.parameters.spot.outerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);
            const float innerDegrees = std::clamp(lightData.parameters.spot.innerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);

            const float outerRadians = XMConvertToRadians(outerDegrees);
            const float innerRadians = XMConvertToRadians(innerDegrees);

            const Vector3 coneDirection = forward * length;

            const float outerBaseRadius = std::tan(outerRadians) * length;
            const float innerBaseRadius = std::tan(innerRadians) * length;

            dd::cone(asFloat3(position), asFloat3(coneDirection), asFloat3(color), outerBaseRadius, 0.0f, 0, depthEnabled);
            dd::cone(asFloat3(position), asFloat3(coneDirection), asFloat3(color), innerBaseRadius, 0.0f, 0, depthEnabled);
            break;
        }
        default:
            break;
        }
    }
}

namespace LightDebugDraw
{
    void drawLightWithoutDepth(const GameObject& gameObject)
    {
        drawLight(gameObject, false);
    }

    void drawLightWithDepth(const GameObject& gameObject)
    {
        drawLight(gameObject, true);
    }
}