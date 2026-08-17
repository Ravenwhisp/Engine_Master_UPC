#include "Globals.h"
#include "VolumetricFogComputePass.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "Texture.h"
#include "VolumetricFogSettings.h"
#include "VolumetricFogTypes.h"

VolumetricFogComputePass::VolumetricFogComputePass(ComPtr<ID3D12Device4> device)
    : m_device(device)
{
}

VolumetricFogComputePass::~VolumetricFogComputePass() = default;

void VolumetricFogComputePass::prepare(const RenderContext& ctx)
{
    Scene* scene = app->getModuleScene()->getScene();

    if (scene == nullptr)
    {
        return;
    }

    const VolumetricFogSettings& settings = scene->getVolumetricFogSettings();

    if (!settings.enabled)
    {
        return;
    }

    ensureVolumes();
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