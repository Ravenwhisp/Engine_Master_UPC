#include "Globals.h"
#include "ShadowCasterCullingPass.h"

#include "Application.h"
#include "ModuleScene.h"
#include "RenderContext.h"
#include "RingBuffer.h"

#include "MeshRenderer.h"
#include "BasicMesh.h"
#include "MeshAsset.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "GameObject.h"
#include "Transform.h"
#include "Skin.h"

#include "ShadowFrustumComputePass.h"

#include <cstring>

namespace
{
    void splitGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS address, uint32_t& low, uint32_t& high)
    {
        low = static_cast<uint32_t>(address & 0xffffffffull);
        high = static_cast<uint32_t>(address >> 32);
    }

    void storeMatrix(const Matrix& matrix, float output[16])
    {
        static_assert(sizeof(Matrix) == sizeof(float) * 16);
        std::memcpy(output, &matrix, sizeof(float) * 16);
    }
}

ShadowCasterCullingPass::ShadowCasterCullingPass(ComPtr<ID3D12Device4> device, ShadowFrustumComputePass* shadowFrustumComputePass)
    : m_device(device)
    , m_shadowFrustumComputePass(shadowFrustumComputePass)
{
}

void ShadowCasterCullingPass::prepare(const RenderContext& ctx)
{
    m_candidateBufferAddress = 0;
    m_candidateCount = 0;

    buildCandidates();

    m_candidateCount = static_cast<uint32_t>(m_candidates.size());

    if (m_candidateCount == 0 || ctx.ringBuffer == nullptr) return;

    m_candidateBufferAddress = ctx.ringBuffer->allocate(
        m_candidates.data(),
        m_candidates.size() * sizeof(ShadowCasterCandidateGPU));

}

void ShadowCasterCullingPass::apply(ID3D12GraphicsCommandList4* commandList)
{
    (void)commandList;
}

void ShadowCasterCullingPass::buildCandidates()
{
    m_candidates.clear();

    for (MeshRenderer* renderer : app->getModuleScene()->getMeshRenderers())
    {
        if (renderer == nullptr || !renderer->isActive() || !renderer->hasMesh()) continue;

        GameObject* owner = renderer->getOwner();
        Transform* transform = renderer->getTransform();

        if (owner == nullptr || !owner->IsActiveInWindowHierarchy() || transform == nullptr) continue;

        const std::shared_ptr<BasicMesh>& mesh = renderer->getMesh();

        if (mesh == nullptr || !mesh->hasIndexBuffer()) continue;

        const Skin* skin = renderer->getSkin();

        const VertexBuffer* gpuSkinnedVB = skin != nullptr ? skin->getCurrentGpuSkinnedVertexBuffer() : nullptr;
        const VertexBuffer* cpuSkinnedVB = skin != nullptr && skin->isCpuSkinningFallbackEnabled() ? skin->getCpuSkinnedVertexBuffer() : nullptr;
        const VertexBuffer* staticVB = mesh->getVertexBuffer().get();

        const bool useGpuSkinnedVB = gpuSkinnedVB != nullptr;
        const bool useCpuSkinnedVB = !useGpuSkinnedVB && cpuSkinnedVB != nullptr;
        const bool useWorldSpaceSkinnedVB = useGpuSkinnedVB || useCpuSkinnedVB;

        const VertexBuffer* activeVB = useGpuSkinnedVB ? gpuSkinnedVB : useCpuSkinnedVB ? cpuSkinnedVB : staticVB;

        if (activeVB == nullptr) continue;

        const Matrix model = useWorldSpaceSkinnedVB ? Matrix::Identity : transform->getGlobalMatrix();
        const Matrix modelTranspose = model.Transpose();

        const Vector3* bounds = renderer->getBoundingBox().getPoints();

        const D3D12_VERTEX_BUFFER_VIEW vbv = activeVB->getVertexBufferView();
        const D3D12_INDEX_BUFFER_VIEW ibv = mesh->getIndexBuffer()->getIndexBufferView();

        for (const Submesh& submesh : mesh->getSubmeshes())
        {
            ShadowCasterCandidateGPU candidate{};

            for (uint32_t i = 0; i < 8; ++i)
            {
                candidate.bounds[i][0] = bounds[i].x;
                candidate.bounds[i][1] = bounds[i].y;
                candidate.bounds[i][2] = bounds[i].z;
                candidate.bounds[i][3] = 1.0f;
            }

            storeMatrix(modelTranspose, candidate.model);

            splitGpuAddress(vbv.BufferLocation, candidate.vertexBufferAddressLow, candidate.vertexBufferAddressHigh);
            candidate.vertexBufferSize = vbv.SizeInBytes;
            candidate.vertexBufferStride = vbv.StrideInBytes;

            splitGpuAddress(ibv.BufferLocation, candidate.indexBufferAddressLow, candidate.indexBufferAddressHigh);
            candidate.indexBufferSize = ibv.SizeInBytes;
            candidate.indexBufferFormat = static_cast<uint32_t>(ibv.Format);

            candidate.indexCountPerInstance = static_cast<uint32_t>(submesh.indexCount);
            candidate.instanceCount = 1;
            candidate.startIndexLocation = static_cast<uint32_t>(submesh.indexStart);
            candidate.baseVertexLocation = 0;
            candidate.startInstanceLocation = 0;

            candidate.flags = skin != nullptr ? SHADOW_CASTER_FLAG_FORCE_VISIBLE : 0u;

            m_candidates.push_back(candidate);
        }
    }
}