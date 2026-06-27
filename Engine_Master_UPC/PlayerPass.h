#pragma once
#include "IRenderPass.h"
#include "Lights.h"
#include "ShadowTypes.h"
#include "LightComponent.h"
#include "SceneDataCB.h"



class MeshRenderer;
class RenderSurface;
struct SceneLightingSettings;

class PlayerPass : public IRenderPass
{
public:
	PlayerPass(ComPtr<ID3D12Device4> device);
	

	virtual void prepare(const RenderContext& ctx) override;
	void apply(ID3D12GraphicsCommandList4* commandList) override;

	GPULightsConstantBuffer packLightsForGPU(const std::vector<LightComponent*>& lights, const Vector3& ambientColor, float ambientIntensity) const;

private:
	void createRootSignature();
	void createPipelineState();

	void renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer);

	ComPtr<ID3D12Device4>		m_device;
	ComPtr<ID3D12RootSignature>	m_rootSignature;
	ComPtr<ID3D12PipelineState>	m_pipelineState;

	D3D12_GPU_VIRTUAL_ADDRESS m_sceneDataCBAddress = 0;
	std::unique_ptr<SceneDataCB> m_sceneDataCB;

	D3D12_GPU_VIRTUAL_ADDRESS m_lightsAddress = 0;
	std::unique_ptr<SceneLightingSettings> m_lighting;
	D3D12_GPU_VIRTUAL_ADDRESS m_shadowCBAddress = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE m_shadowMapSRV{};
	bool m_hasShadowData = false;

	std::vector<MeshRenderer*>	m_meshRenderers;

	D3D12_VIEWPORT              m_viewport = {};
	D3D12_RECT                  m_scissorRect = {};

	const Matrix* m_view = nullptr;
	const Matrix* m_projection = nullptr;

	RenderSurface* m_renderSurface = nullptr;
public:
};