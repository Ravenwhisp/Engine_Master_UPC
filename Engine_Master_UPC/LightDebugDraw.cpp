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
            //The implementation using dd::cone is bugged and I cannot solve it. 

            /*const float length = lightData.parameters.spot.radius;

            const float outerDegrees = std::clamp(lightData.parameters.spot.outerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);
            const float innerDegrees = std::clamp(lightData.parameters.spot.innerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);

            const float outerRadians = XMConvertToRadians(outerDegrees);
            const float innerRadians = XMConvertToRadians(innerDegrees);

            const Vector3 coneDirection = forward * length;

            const float outerBaseRadius = std::tan(outerRadians) * length;
            const float innerBaseRadius = std::tan(innerRadians) * length;

            dd::cone(asFloat3(position), asFloat3(coneDirection), asFloat3(color), outerBaseRadius, 0.0f, 0, depthEnabled);
            dd::cone(asFloat3(position), asFloat3(coneDirection), asFloat3(color), innerBaseRadius, 0.0f, 0, depthEnabled);
            break;*/

            const float length = lightData.parameters.spot.radius;

            float outerDegrees = std::clamp(lightData.parameters.spot.outerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);
            float innerDegrees = std::clamp(lightData.parameters.spot.innerAngleDegrees, 0.0f, SPOT_DEBUG_MAX_ANGLE_DEGREES);

            float outerRadians = XMConvertToRadians(outerDegrees);
            float innerRadians = XMConvertToRadians(innerDegrees);

            Vector3 direction = forward;
            direction.Normalize();

            Vector3 referenceAxis;

            if (std::abs(direction.y) > 0.99f)
            {
                referenceAxis = Vector3(1.0f, 0.0f, 0.0f);
            }
            else
            {
                referenceAxis = Vector3(0.0f, 1.0f, 0.0f);
            }

            Vector3 x = referenceAxis.Cross(direction);
            x.Normalize();

            Vector3 y = direction.Cross(x);   
            y.Normalize();

            auto drawConeWire = [&](float angleRad)
                {
                    float baseCircleRadius = std::tan(angleRad) * length;
                    Vector3 baseCenter = position + direction * length;

                    const int stepDegrees = 20;
                    Vector3 previousCirclePoint = baseCenter + y * baseCircleRadius;

                    for (int angleDegrees = stepDegrees; angleDegrees <= 360; angleDegrees += stepDegrees)
                    {
                        float sin = std::sin(XMConvertToRadians((float)angleDegrees));
                        float cos = std::cos(XMConvertToRadians((float)angleDegrees));
                        Vector3 circlePoint = baseCenter + (x * sin + y * cos) * baseCircleRadius;

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