#pragma once

#include "IRenderPass.h"
#include "OutlineSettings.h"
#include "DescriptorHandle.h"
#include "DescriptorHeapBlock.h"
#include "SimpleMath.h"
#include "AssetReference.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

namespace DirectX { namespace SimpleMath { struct Matrix; } }
using Matrix = DirectX::SimpleMath::Matrix;

class Texture;

class OutlinePass : public IRenderPass
{
public:
	OutlinePass(ComPtr<ID3D12Device4> device);
	~OutlinePass();

	void prepare(const RenderContext& ctx) override;
	void apply(ID3D12GraphicsCommandList4* commandList) override;

private:
	struct CachedColorCopy
	{
		ComPtr<ID3D12Resource> resource;
		DescriptorHandle       srv;
	};

	void createRootSignature();
	void createPipelineState();
	void releaseManualSRV();
	void releaseCopyResources();
	void ensureColorCopy(uint32_t width, uint32_t height, DXGI_FORMAT format);
	void ensureFallbackTexture();
	void loadNoiseTexture(const AssetReference& assetId);

	static uint64_t makeCopyKey(uint32_t w, uint32_t h) { return (uint64_t(w) << 32) | uint64_t(h); }

	ComPtr<ID3D12Device4>       m_device;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	const Texture*              m_depthTexture = nullptr;
	const Texture*              m_colorTexture = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE m_depthSRV = {};
	OutlineSettings             m_cachedSettings;
	Matrix                      m_invProjection = Matrix::Identity;

	DescriptorHandle            m_manualSRV = {};
	bool                        m_hasManualSRV = false;

	std::unordered_map<uint64_t, CachedColorCopy> m_colorCopyCache;
	uint64_t                    m_activeCopyKey = 0;

	DescriptorHandle            m_noiseSRV;
	bool                        m_hasManualNoiseSRV = false;

	std::unordered_map<uint64_t, DescriptorHeapBlock*> m_srvBlockCache;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvTableGpu = {};

	ComPtr<ID3D12Resource>      m_fallbackTexture;
	DescriptorHandle            m_fallbackSRV;
	bool                        m_hasFallbackSRV = false;

	AssetReference              m_loadedNoiseAssetId{};
	std::shared_ptr<Texture>    m_loadedNoiseTexture;

	bool  m_enabled = false;
	float m_viewportWidth = 0.0f;
	float m_viewportHeight = 0.0f;
};
