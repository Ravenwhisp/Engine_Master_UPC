#include "Globals.h"

#include "PlayerPass.h"

#include "BasicMaterial.h"

#include "ModuleDescriptors.h"

PlayerPass::PlayerPass(ComPtr<ID3D12Device4> device)
{
	m_device = device;

	createRootSignature();
	createPipelineState();
}

void PlayerPass::createRootSignature()
{
	CD3DX12_ROOT_PARAMETER		rootParams[11] = {};
	CD3DX12_DESCRIPTOR_RANGE	srvRange, irradianceRange, brdfRange, sampRange, prefilteredRange, shadowMapRange;

    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
    irradianceRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
    brdfRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);
    prefilteredRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
    shadowMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11, 0);

    rootParams[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParams[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParams[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParams[4].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[5].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[6].InitAsDescriptorTable(1, &irradianceRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[7].InitAsDescriptorTable(1, &prefilteredRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[8].InitAsDescriptorTable(1, &brdfRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[9].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[10].InitAsDescriptorTable(1, &shadowMapRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> sigBlob, errorBlob;
	DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob));
	DXCall(m_device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void PlayerPass::createPipelineState()
{
}

void PlayerPass::prepare(const RenderContext& ctx)
{

}

void PlayerPass::apply(ID3D12GraphicsCommandList4* commandList)
{
}


void PlayerPass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer)
{
}
