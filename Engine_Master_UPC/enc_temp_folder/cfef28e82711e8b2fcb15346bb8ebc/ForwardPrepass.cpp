#include "Globals.h"
#include "ForwardPrepass.h"
#include "Application.h"

#include "ModuleScene.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleRender.h"

#include "SceneDataCB.h"
#include "RingBuffer.h"
#include "BasicMaterial.h"
#include "RenderContext.h"
#include "GameObject.h"
#include "Transform.h"
#include "IndexBuffer.h"

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

	rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Init(_countof(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> sigBlob, errorBlob;
	DXCall(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob));
	DXCall(m_device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void ForwardPrepass::createPipelineState()
{
    ComPtr<ID3DBlob> vsBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"ForwardPrepassVertexShader.cso", &vsBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    psoDesc.PS = {};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 0;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
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
    psoDesc.SampleDesc.Count = 1;

    ThrowIfFailed( m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
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
	BEGIN_EVENT(commandList, "ForwardPrepass");

	auto depthStencilTex = m_renderSurface->getTexture(RenderSurface::DEPTH_STENCIL);
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthStencilTex->getDSV().cpu;
	commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	commandList->OMSetStencilRef(1);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto* renderer : m_meshRenderers)
	{
		renderMeshRenderer(commandList, renderer);
	}

	END_EVENT(commandList);
}

void ForwardPrepass::renderMeshRenderer(ID3D12GraphicsCommandList4* commandList, MeshRenderer* renderer)
{
	GameObject* owner = renderer->getOwner();
	if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
	{
		return;
	}

	if (!renderer->isActive())
	{
		return;
	}

	Transform* transform = renderer->getTransform();

	const auto& mesh = renderer->getMesh();
	if (mesh.get() == nullptr)
	{
		return;
	}

	const auto& submeshes = mesh->getSubmeshes();
	const auto& materials = renderer->getMaterials();

	if (materials.size() != submeshes.size())
	{
		return;
	}

	const Skin* skin = renderer->getSkin();

	const VertexBuffer* gpuSkinnedVB = skin ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
	const VertexBuffer* cpuSkinnedVB = skin && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;

	const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

	const bool useGpuSkinnedVB = (gpuSkinnedVB != nullptr);
	const bool useCpuSkinnedVB = (!useGpuSkinnedVB && cpuSkinnedVB != nullptr);
	const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

	const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : (useCpuSkinnedVB ? cpuSkinnedVB : staticVB);

	if (!activeVB)
	{
		return;
	}

	Matrix global = transform->getGlobalMatrix();
	Matrix mvp = useWorldSpaceSkinnedVB ? (*m_view * *m_projection).Transpose() : (global * *m_view * *m_projection).Transpose();

	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(UINT32), &mvp, 0);

	for (int i = 0; i < submeshes.size(); i++)
	{
		const auto& material = materials.at(i).get();

		commandList->SetGraphicsRootConstantBufferView(0, app->getModuleRender()->allocateInRingBuffer(&mvp, sizeof(Matrix)));

		D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
		commandList->IASetVertexBuffers(0, 1, &vbv);

		if (mesh->hasIndexBuffer())
		{
			D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
			commandList->IASetIndexBuffer(&ibv);

			commandList->DrawIndexedInstanced(submeshes.at(i).indexCount, 1, submeshes.at(i).indexStart, 0, 0);
		}
	}
}