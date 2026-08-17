#include "Globals.h"
#include "VolumetricFogComputePass.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleScene.h"

#include "RenderContext.h"
#include "Scene.h"
#include "Texture.h"
#include "VolumetricFogSettings.h"
#include "VolumetricFogTypes.h"

#include <cmath>


namespace
{
    bool getCameraNearDistance(const Matrix& projection, float& nearDistance)
    {
        if (std::abs(projection._33) <= 0.000001f) return false;

        nearDistance = std::abs(projection._43 / projection._33);
        return nearDistance > 0.0f;
    }
}

VolumetricFogComputePass::VolumetricFogComputePass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
}

VolumetricFogComputePass::~VolumetricFogComputePass() = default;

void VolumetricFogComputePass::prepare(const RenderContext& ctx)
{
    m_enabled = false;
    m_gridConstants = {};

    Scene* scene = app->getModuleScene()->getScene();
    if (scene == nullptr) return;

    const VolumetricFogSettings& settings = scene->getVolumetricFogSettings();
    if (!settings.enabled) return;

    float nearDistance = 0.0f;
    if (!getCameraNearDistance(ctx.projection, nearDistance)) return;

    const float maxDistance = std::max(settings.maxDistance, nearDistance + VolumetricFog::MIN_DEPTH_RANGE);
    const float projectionScaleX = ctx.projection._11;
    const float projectionScaleY = ctx.projection._22;

    if (std::abs(projectionScaleX) <= 0.000001f || std::abs(projectionScaleY) <= 0.000001f) return;

    ensureVolumes();

    m_gridConstants.inverseView = ctx.view.Invert().Transpose();
    m_gridConstants.projectionScale = Vector2(projectionScaleX, projectionScaleY);
    m_gridConstants.nearDistance = nearDistance;
    m_gridConstants.maxDistance = maxDistance;
    m_gridConstants.gridWidth = VolumetricFog::GRID_WIDTH;
    m_gridConstants.gridHeight = VolumetricFog::GRID_HEIGHT;
    m_gridConstants.gridDepth = VolumetricFog::GRID_DEPTH;

    m_enabled = m_mediumVolume != nullptr && m_lightingVolume != nullptr && m_integratedVolume != nullptr;
}

void VolumetricFogComputePass::apply(ID3D12GraphicsCommandList4* commandList)
{
    
}

void VolumetricFogComputePass::ensureVolumes()
{
    if (m_mediumVolume == nullptr)
    {
        m_mediumVolume.reset(app->getModuleResources()->createVolumeTexture(
                VolumetricFog::GRID_WIDTH,
                VolumetricFog::GRID_HEIGHT,
                VolumetricFog::GRID_DEPTH,
                DXGI_FORMAT_R16G16B16A16_FLOAT));

        m_mediumVolume->setName(L"VolumetricFog_Medium");
    }

    if (m_lightingVolume == nullptr)
    {
        m_lightingVolume.reset(app->getModuleResources()->createVolumeTexture(
                VolumetricFog::GRID_WIDTH,
                VolumetricFog::GRID_HEIGHT,
                VolumetricFog::GRID_DEPTH,
                DXGI_FORMAT_R16G16B16A16_FLOAT));

        m_lightingVolume->setName(L"VolumetricFog_Lighting");
    }

    if (m_integratedVolume == nullptr)
    {
        m_integratedVolume.reset(app->getModuleResources()->createVolumeTexture(
                VolumetricFog::GRID_WIDTH,
                VolumetricFog::GRID_HEIGHT,
                VolumetricFog::GRID_DEPTH,
                DXGI_FORMAT_R16G16B16A16_FLOAT));

        m_integratedVolume->setName(L"VolumetricFog_Integrated");
    }
}