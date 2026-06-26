#include "Globals.h"
#include "RimErosionPass.h"

#include "RenderContext.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "ModuleRender.h"
#include "ModuleScene.h"
#include "SkyBoxPass.h"

#include "Scene.h"
#include "SceneLightingSettings.h"
#include "SceneDataCB.h"
#include "GameObject.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "Skin.h"
#include "LightComponent.h"
#include "RingBuffer.h"
#include "VertexBuffer.h"
#include "ModuleAssets.h"
#include "Texture.h"
#include "TextureAsset.h"
#include "BasicMesh.h"
#include "SkyBox.h"
#include "Settings.h"

#include "SimpleMath.h"
#include <d3dcompiler.h>
#include "PlatformHelpers.h"
#include "OptickProfiler.h"

#include <IndexBuffer.h>
#include <DescriptorHeapBlock.h>

RimErosionPass::RimErosionPass(ComPtr<ID3D12Device4> device) : m_device(device)
{
	m_lighting = std::make_unique<SceneLightingSettings>();
	m_sceneDataCB = std::make_unique<SceneDataCB>();

	m_lighting->ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
	m_lighting->ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	CD3DX12_ROOT_PARAMETER rootParameters[13] = {};
	CD3DX12_DESCRIPTOR_RANGE srvRange, irradianceRange, brdfRange, sampRange, prefilteredRange;
	CD3DX12_DESCRIPTOR_RANGE shadowMapRange, brushRange;
	brushRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 12, 0);
	shadowMapRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 11, 0);

	srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, BasicMaterial::SLOT_COUNT, 0, 0);
	irradianceRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
	prefilteredRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
	brdfRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0);
	sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleDescriptors::SampleType::COUNT, 0);

	rootParameters[0].InitAsConstants((sizeof(Transforms) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[3].InitAsConstantBufferView(3, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[4].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[5].InitAsDescriptorTable(1, &irradianceRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[6].InitAsDescriptorTable(1, &prefilteredRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[7].InitAsDescriptorTable(1, &brdfRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[8].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// Shadows
	rootParameters[9].InitAsConstantBufferView(4, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[10].InitAsDescriptorTable(1, &shadowMapRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// Brush texture (t12)
	rootParameters[11].InitAsDescriptorTable(1, &brushRange, D3D12_SHADER_VISIBILITY_PIXEL);

	// Erosion data CBV (b5) — use CBV instead of root constants to stay within 64-DWORD root signature budget
	rootParameters[12].InitAsConstantBufferView(5, 0, D3D12_SHADER_VISIBILITY_ALL);

	rootSignatureDesc.Init(13, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	DXCall(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DXCall(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));


#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"RimErosionVertexShader.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"RimErosionPixelShader.cso", &pixelShaderBlob));

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = TRUE;

	// Alpha blending for overlay
	D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
	rtBlendDesc.BlendEnable = TRUE;
	rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.RenderTarget[0] = rtBlendDesc;

	// Read depth but don't write (render on top of PBR pass)
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState = depthStencilDesc;

	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = { 1,0 };

	DXCall(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	// Allocate a descriptor block in the global SRV heap for the brush texture
	DescriptorHeap& srvHeap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_brushBlock = srvHeap.allocateBlock(1);

	// Write a null SRV so t12 is always bound (sampling returns 0 when no texture)
	D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
	nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_device->CreateShaderResourceView(nullptr, &nullSrvDesc, m_brushBlock->getCPUHandle(0));
	m_brushTextureSRV = m_brushBlock->getGPUHandle(0);
}

RimErosionPass::~RimErosionPass()
{
}

void RimErosionPass::prepare(const RenderContext& ctx)
{
	PERF_RENDER("RimErosionPass::prepare");

	{
		PERF_RENDER("RimErosionPass::prepare::SetupCamera");
		m_view = &ctx.view;
		m_projection = &ctx.projection;
		m_sceneDataCB->viewPos = ctx.cameraPosition;
	}

	{
		PERF_RENDER("RimErosionPass::prepare::GetVisibleMeshRenderers");
		m_meshRenderers = app->getModuleScene()->getVisibleMeshRenderers();
	}

	{
		PERF_RENDER("RimErosionPass::prepare::UploadSceneDataCB");
		m_sceneDataCBAddress = ctx.ringBuffer->allocate(
			m_sceneDataCB.get(),
			sizeof(SceneDataCB),
			app->getModuleD3D12()->getCurrentFrame());
	}

	GPULightsConstantBuffer lightsCB{};
	{
		PERF_RENDER("RimErosionPass::prepare::PackLights");
		lightsCB = packLightsForGPU(
			app->getModuleScene()->getLightComponents(),
			m_lighting->ambientColor,
			m_lighting->ambientIntensity);
	}

	{
		PERF_RENDER("RimErosionPass::prepare::UploadLightsCB");
		m_lightsAddress = ctx.ringBuffer->allocate(
			&lightsCB,
			sizeof(GPULightsConstantBuffer),
			app->getModuleD3D12()->getCurrentFrame());
	}

	m_hasShadowData = ctx.shadowData != nullptr;

	if (m_hasShadowData)
	{
		m_shadowCBAddress = ctx.shadowData->shadowCBAddress;
		m_shadowMapSRV = ctx.shadowData->shadowMapSRV;
	}
	else
	{
		m_shadowCBAddress = 0;
		m_shadowMapSRV = {};
	}

	// Prepare erosion data (passed as root constants)
	{
		PERF_RENDER("RimErosionPass::prepare::PrepareErosionData");

		const auto& erosionSettings = app->getSettings()->rimErosion;

		m_erosionData.displacementAmount = erosionSettings.displacementAmount;
		m_erosionData.rimThreshold = erosionSettings.rimThreshold;
		m_erosionData.rimSoftness = erosionSettings.rimSoftness;
		m_erosionData.erosionIntensity = erosionSettings.erosionIntensity;
		m_erosionData.brushScale = erosionSettings.brushScale;
		m_erosionData.brushOffsetX = erosionSettings.brushOffsetX;
		m_erosionData.brushOffsetY = erosionSettings.brushOffsetY;
		m_erosionData.preserveSilhouette = erosionSettings.preserveSilhouette;
		m_erosionData.erosionColor[0] = erosionSettings.erosionColor[0];
		m_erosionData.erosionColor[1] = erosionSettings.erosionColor[1];
		m_erosionData.erosionColor[2] = erosionSettings.erosionColor[2];
		m_erosionData.erosionColor[3] = 1.0f;
		m_erosionData.debugRimMask = erosionSettings.debugRimMask ? 1.0f : 0.0f;
		m_erosionData.pad0 = 0.0f;
		m_erosionData.pad1 = 0.0f;
		m_erosionData.pad2 = 0.0f;
		m_erosionData.paintColor1[0] = erosionSettings.paintColor1[0];
		m_erosionData.paintColor1[1] = erosionSettings.paintColor1[1];
		m_erosionData.paintColor1[2] = erosionSettings.paintColor1[2];
		m_erosionData.paintColor1[3] = 1.0f;
		m_erosionData.paintColor2[0] = erosionSettings.paintColor2[0];
		m_erosionData.paintColor2[1] = erosionSettings.paintColor2[1];
		m_erosionData.paintColor2[2] = erosionSettings.paintColor2[2];
		m_erosionData.paintColor2[3] = 1.0f;
		m_erosionData.brushNormalStrength = erosionSettings.brushNormalStrength;
		m_erosionData.curvatureScale = erosionSettings.curvatureScale;
		m_erosionData.toonSharpness = erosionSettings.toonSharpness;
		m_erosionData.pad3 = 0.0f;

		m_erosionCBAddress = ctx.ringBuffer->allocate(
			&m_erosionData,
			sizeof(ErosionGPUData),
			app->getModuleD3D12()->getCurrentFrame());
	}

	// Load brush texture if needed
	{
		PERF_RENDER("RimErosionPass::prepare::LoadBrushTexture");

		const auto& erosionSettings = app->getSettings()->rimErosion;
		if (erosionSettings.brushTextureAssetId.isValid())
		{
			if (erosionSettings.brushTextureAssetId != m_cachedBrushAssetRef)
			{
				m_cachedBrushAssetRef = erosionSettings.brushTextureAssetId;
				m_brushTexture.reset();

				AssetReference brushRef = erosionSettings.brushTextureAssetId;
				auto textureAsset = app->getModuleAssets()->load<TextureAsset>(brushRef);
				if (textureAsset)
				{
					m_brushTexture = app->getModuleResources()->createTexture(*textureAsset);
					if (m_brushTexture && m_brushBlock)
					{
						D3D12_CPU_DESCRIPTOR_HANDLE src = m_brushTexture->getSRV().cpu;
						D3D12_CPU_DESCRIPTOR_HANDLE dst = m_brushBlock->getCPUHandle(0);
						app->getModuleD3D12()->getDevice()->CopyDescriptorsSimple(1, dst, src, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
						m_brushTextureSRV = m_brushBlock->getGPUHandle(0);
					}
				}
			}
		}
		else
		{
			m_cachedBrushAssetRef = AssetReference{};
			m_brushTexture.reset();
			// Keep null SRV bound (t12 stays valid, sampling returns 0)
			m_brushTextureSRV = m_brushBlock->getGPUHandle(0);
		}
	}
}

void RimErosionPass::apply(ID3D12GraphicsCommandList4* commandList)
{
	if (!app->getSettings()->rimErosion.enabled)
		return;

	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).getHeap(), app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootConstantBufferView(1, m_sceneDataCBAddress);
	commandList->SetGraphicsRootConstantBufferView(3, m_lightsAddress);

	commandList->SetGraphicsRootDescriptorTable(8, app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER).getGPUHandle(ModuleDescriptors::SampleType::LINEAR_WRAP));

	// Bind erosion data as CBV
	commandList->SetGraphicsRootConstantBufferView(12, m_erosionCBAddress);

	// Bind brush texture (always bound, null SRV when no texture)
	commandList->SetGraphicsRootDescriptorTable(11, m_brushTextureSRV);

	if (m_hasShadowData && m_shadowCBAddress != 0 && m_shadowMapSRV.ptr != 0)
	{
		commandList->SetGraphicsRootConstantBufferView(9, m_shadowCBAddress);
		commandList->SetGraphicsRootDescriptorTable(10, m_shadowMapSRV);
	}

	renderRimErosion(commandList);
}


void RimErosionPass::renderRimErosion(ID3D12GraphicsCommandList* commandList)
{
	PERF_RENDER("RimErosionPass::renderRimErosion");

	for (const auto& renderer : m_meshRenderers)
	{
		{
			PERF_RENDER("RimErosionPass::renderRimErosion::RendererValidation");

			if (!renderer->hasShaderFlag(MeshRenderer::ShaderType::SHADER_RIM_EROSION))
				continue;

			GameObject* owner = renderer->getOwner();
			if (owner == nullptr || !owner->IsActiveInWindowHierarchy())
			{
				continue;
			}

			if (!renderer->isActive())
			{
				continue;
			}
		}

		Transform* transform = renderer->getTransform();

		const auto& mesh = renderer->getMesh();
		if (mesh.get() == nullptr)
			continue;

		const auto& submeshes = mesh->getSubmeshes();
		const auto& materials = renderer->getMaterials();

		if (materials.size() != submeshes.size())
			continue;

		{
			PERF_RENDER("RimErosionPass::renderRimErosion::VertexBufferSelection");

			const Skin* skin = renderer->getSkin();

			const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

			const VertexBuffer* gpuSkinnedVB = skin ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
			const VertexBuffer* cpuSkinnedVB = skin && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;

			const bool useGpuSkinnedVB = (gpuSkinnedVB != nullptr);
			const bool useCpuSkinnedVB = (!useGpuSkinnedVB && cpuSkinnedVB != nullptr);
			const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

			const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : (useCpuSkinnedVB ? cpuSkinnedVB : staticVB);

			if (!activeVB)
				continue;

			Matrix global = transform->getGlobalMatrix();
			struct Transforms transforms = {};
			transforms.mvp = useWorldSpaceSkinnedVB ? (*m_view * *m_projection).Transpose() : (global * *m_view * *m_projection).Transpose();
			transforms.nm = global.Invert().Transpose();

			commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) * 2 / sizeof(UINT32), &transforms, 0);

			{
				PERF_RENDER("RimErosionPass::renderRimErosion::SubmeshLoop");

				for (int i = 0; i < submeshes.size(); i++)
				{
					const auto& material = materials.at(i).get();

					{
						PERF_RENDER("RimErosionPass::renderRimErosion::ModelDataUpload");
						ModelData modelData{};
						modelData.model = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getGlobalMatrix().Transpose();
						modelData.normalMat = useWorldSpaceSkinnedVB ? Matrix::Identity.Transpose() : transform->getNormalMatrix().Transpose();
						modelData.material = material->getMaterial();

						commandList->SetGraphicsRootConstantBufferView(2, app->getModuleRender()->allocateInRingBuffer(&modelData, sizeof(ModelData))
						);
					}

					{
						PERF_RENDER("RimErosionPass::renderRimErosion::BindMaterial");
						commandList->SetGraphicsRootDescriptorTable(4, material->getTableGPUHandle());
						commandList->SetGraphicsRootDescriptorTable(5, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getIrradiance()->getSRV().gpu);
						commandList->SetGraphicsRootDescriptorTable(6, app->getModuleRender()->getSkyBoxPass()->getSkyBox()->getEnvironment()->getSRV().gpu);
						commandList->SetGraphicsRootDescriptorTable(7, app->getModuleResources()->getEnvironmentBrdfTexture()->getSRV().gpu);

						commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

						D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
						commandList->IASetVertexBuffers(0, 1, &vbv);
					}

					if (mesh->hasIndexBuffer())
					{
						PERF_RENDER("RimErosionPass::renderRimErosion::DrawIndexed");
						D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();
						commandList->IASetIndexBuffer(&ibv);
						commandList->DrawIndexedInstanced(submeshes.at(i).indexCount, 1, submeshes.at(i).indexStart, 0, 0);
					}
				}
			}
		}
	}
}

GPULightsConstantBuffer RimErosionPass::packLightsForGPU(
	const std::vector<LightComponent*>& lights,
	const Vector3& ambientColor,
	float ambientIntensity) const
{
	GPULightsConstantBuffer cb{};
	cb.ambientColor = ambientColor;
	cb.ambientIntensity = ambientIntensity;

	for (const LightComponent* light : lights)
	{
		if (!light->isActive())
			continue;

		const GameObject* owner = light->getOwner();
		if (!owner || !owner->IsActiveInWindowHierarchy())
			continue;

		const Transform* transform = owner->GetTransform();
		if (!transform)
			continue;

		const LightData& data = light->getData();
		const LightCommon& common = data.common;
		const Matrix& world = transform->getGlobalMatrix();
		const Vector3      pos(world._41, world._42, world._43);
		const Vector3      fwd = transform->getForward();

		switch (data.type)
		{
		case LightType::DIRECTIONAL:
			if (cb.directionalCount < LightDefaults::MAX_DIRECTIONAL_LIGHTS)
			{
				auto& l = cb.directionalLights[cb.directionalCount++];
				l.direction = fwd;
				l.color = common.color;
				l.intensity = common.intensity;
			}
			break;

		case LightType::POINT:
			if (cb.pointCount < LightDefaults::MAX_POINT_LIGHTS)
			{
				auto& l = cb.pointLights[cb.pointCount++];
				l.position = pos;
				l.radius = data.parameters.point.radius;
				l.color = common.color;
				l.intensity = common.intensity;
			}
			break;

		case LightType::SPOT:
			if (cb.spotCount < LightDefaults::MAX_SPOT_LIGHTS)
			{
				const auto& sp = data.parameters.spot;
				auto& l = cb.spotLights[cb.spotCount++];
				l.position = pos;
				l.direction = fwd;
				l.radius = sp.radius;
				l.color = common.color;
				l.intensity = common.intensity;
				l.cosineInnerAngle = std::cos(XMConvertToRadians(sp.innerAngleDegrees));
				l.cosineOuterAngle = std::cos(XMConvertToRadians(sp.outerAngleDegrees));
			}
			break;

		default: break;
		}
	}

	return cb;
}
