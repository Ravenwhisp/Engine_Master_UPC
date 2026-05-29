#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

class ModuleResources;
class Texture;
class SkyBox;

class EquirectangularPass
{
public:
	EquirectangularPass(ComPtr<ID3D12Device4> device);

	
	Texture* renderCubemap(SkyBox* skybox);

private:
	ComPtr<ID3D12Device4>		m_device;
	ComPtr<ID3D12RootSignature>	m_rootSignature;
	ComPtr<ID3D12PipelineState>	m_pipelineState;

};

