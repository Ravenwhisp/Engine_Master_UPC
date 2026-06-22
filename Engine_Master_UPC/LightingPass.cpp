#include "Globals.h"
#include "LightingPass.h"
#include "GameObject.h"
#include "Transform.h"
#include "LightComponent.h"

LightingPass::LightingPass(ComPtr<ID3D12Device4> device)
{
}

void LightingPass::prepare(const RenderContext& ctx)
{
}

void LightingPass::apply(ID3D12GraphicsCommandList4* commandList)
{
}

GPULightsConstantBuffer LightingPass::packLightsForGPU(  const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const
{
    GPULightsConstantBuffer cb{};
    cb.ambientColor = ambientColor;
    cb.ambientIntensity = ambientIntensity;

    for (const LightComponent* light : lights)
    {
        if (!light->isActive())
            continue;

        const GameObject* owner = light->getOwner();
        if (!owner || !owner->IsActiveInWindowHierarchy())
            continue;

        const Transform* transform = owner->GetTransform();
        if (!transform)
            continue;

        const LightData& data = light->getData();
        const LightCommon& common = data.common;
        const Matrix& world = transform->getGlobalMatrix();
        const Vector3      pos(world._41, world._42, world._43);
        const Vector3      fwd = transform->getForward();

        switch (data.type)
        {
        case LightType::DIRECTIONAL:
            if (cb.directionalCount < LightDefaults::MAX_DIRECTIONAL_LIGHTS)
            {
                auto& l = cb.directionalLights[cb.directionalCount++];
                l.direction = fwd;
                l.color = common.color;
                l.intensity = common.intensity;
            }
            break;

        case LightType::POINT:
            if (cb.pointCount < LightDefaults::MAX_POINT_LIGHTS)
            {
                auto& l = cb.pointLights[cb.pointCount++];
                l.position = pos;
                l.radius = data.parameters.point.radius;
                l.color = common.color;
                l.intensity = common.intensity;
            }
            break;

        case LightType::SPOT:
            if (cb.spotCount < LightDefaults::MAX_SPOT_LIGHTS)
            {
                const auto& sp = data.parameters.spot;
                auto& l = cb.spotLights[cb.spotCount++];
                l.position = pos;
                l.direction = fwd;
                l.radius = sp.radius;
                l.color = common.color;
                l.intensity = common.intensity;
                l.cosineInnerAngle = std::cos(XMConvertToRadians(sp.innerAngleDegrees));
                l.cosineOuterAngle = std::cos(XMConvertToRadians(sp.outerAngleDegrees));
            }
            break;

        default: break;
        }
    }

    return cb;
}