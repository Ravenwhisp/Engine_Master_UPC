#pragma once

#include "IRenderPass.h"

#include <d3d12.h>
#include <memory>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Texture;

class VolumetricFogComputePass final : public IRenderPass
{
public:
    explicit VolumetricFogComputePass(ComPtr<ID3D12Device4> device);
    ~VolumetricFogComputePass() override;

    void prepare(const RenderContext& ctx) override;
    void apply(ID3D12GraphicsCommandList4* commandList) override;

    const Texture* getMediumVolume() const { return m_mediumVolume.get(); }
    const Texture* getLightingVolume() const { return m_lightingVolume.get(); }
    const Texture* getIntegratedVolume() const { return m_integratedVolume.get(); }

    Texture* getMediumVolume() { return m_mediumVolume.get(); }
    Texture* getLightingVolume() { return m_lightingVolume.get(); }
    Texture* getIntegratedVolume() { return m_integratedVolume.get(); }

private:
    void ensureVolumes();

private:
    ComPtr<ID3D12Device4> m_device;

    std::unique_ptr<Texture> m_mediumVolume;
    std::unique_ptr<Texture> m_lightingVolume;
    std::unique_ptr<Texture> m_integratedVolume;
};