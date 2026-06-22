#pragma once

#include "IRenderPass.h"
#include "OutlineSettings.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Texture;

class OutlinePass : public IRenderPass
{
public:
	OutlinePass(ComPtr<ID3D12Device4> device);

	void prepare(const RenderContext& ctx) override;
	void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
	void createRootSignature();
	void createPipelineState();

	ComPtr<ID3D12Device4>       m_device;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	const Texture*              m_depthTexture = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE m_depthSRV = {};
	OutlineSettings             m_cachedSettings;

	bool  m_enabled = false;
	float m_viewportWidth = 0.0f;
	float m_viewportHeight = 0.0f;
};
