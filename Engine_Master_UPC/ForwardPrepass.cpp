#include "Globals.h"
#include "ForwardPrepass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "SceneDataCB.h"
#include "RingBuffer.h"
#include "ModuleD3D12.h"
#include "BasicMaterial.h"
#include "ModuleDescriptors.h"
#include "RenderContext.h"

#include "PlatformHelpers.h"

#include <d3dcompiler.h>


ForwardPrepass::ForwardPrepass(ComPtr<ID3D12Device4> device)
{
	createRootSignature();
	createPipelineState();
}

void ForwardPrepass::createRootSignature()
{
	CD3DX12_ROOT_PARAMETER		rootParams[1] = {};
	CD3DX12_DESCRIPTOR_RANGE	srvRange, sampRange;

	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
	sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

	rootParams[0].InitAsConstants(sizeof(Matrix) / sizeof(UINT32), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> sigBlob, errorBlob;
	DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob));
	DXCall(m_device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void ForwardPrepass::createPipelineState()
{
	ComPtr<ID3DBlob> psBlob, vsBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"", &psBlob));
	ThrowIfFailed(D3DReadFileToBlob(L"", &vsBlob));

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCORD",  0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = TRUE;
	psoDesc.DepthStencilState.StencilReadMask = 0xFF;
	psoDesc.DepthStencilState.StencilWriteMask = 0xFF;
	psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	psoDesc.DepthStencilState.BackFace = psoDesc.DepthStencilState.FrontFace;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDesc.SampleDesc = { 1, 0 };

	DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void ForwardPrepass::prepare(const RenderContext& ctx)
{
	m_view = &ctx.view;
	m_projection = &ctx.projection;

	m_viewport = ctx.viewport;
	m_scissorRect = ctx.scissorRect;

	// Collect visible mesh renderers
	m_meshRenderers = app->getModuleScene()->getVisibleMeshRenderers(RenderMode::DEFERRED);

	// Upload SceneDataCB (camera position) to the ring buffer
	SceneDataCB sceneData{};
	sceneData.viewPos = ctx.cameraPosition;

	m_sceneDataCBAddress = ctx.ringBuffer->allocate(&sceneData, sizeof(SceneDataCB), app->getModuleD3D12()->getCurrentFrame());

	m_renderSurface = &ctx.renderSurface;
}

void ForwardPrepass::apply(ID3D12GraphicsCommandList4* commandList)
{
	BEGIN_EVENT(commandList, "Deferred");


	auto colorTex = m_renderSurface->getTexture(RenderSurface::COMPOSITE);
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = colorTex->getRTV(0).cpu;
	commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	// Bind root signature (must be set before any draw calls)
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	//Set input assembler
	ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->OMSetStencilRef(1);
}