#pragma once

#include "MD5Fwd.h"
#include "SimpleMath.h"
#include "Vertex.h"
#include "VertexBuffer.h"
#include "Settings.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <memory>
#include <vector>

#include "AssetReference.h"

using Microsoft::WRL::ComPtr;

class GameObject;
class MeshRenderer;
class MeshAsset;
class SkinAsset;
class Transform;

class Skin
{
public:
    Skin() = default;
    ~Skin() = default;

    std::unique_ptr<Skin> clone() const;

    void lateUpdate(GameObject* owner, MeshRenderer& renderer);
    void cleanUp();

    void setSkinReference(AssetReference& skinUID);
    AssetReference& getSkinReference() { return m_skinAsset; }

    const std::shared_ptr<SkinAsset>& getSkinAsset() const { return m_skin; }
    const std::vector<Transform*>& getJointTransforms() const { return m_jointTransforms; }

    bool hasResolvedSkinBindings() const { return m_skinBindingsResolved; }
    uint32_t getResolvedJointCount() const { return static_cast<uint32_t>(m_jointTransforms.size()); }

    const std::vector<Matrix>& getMatrixPalette() const { return m_matrixPalette; }
    const std::vector<Matrix>& getNormalPalette() const { return m_normalPalette; }
    bool hasSkinPalette() const { return !m_matrixPalette.empty(); }

    uint32_t getSkinningVertexCount() const { return static_cast<uint32_t>(m_sourceVertices.size()); }

    const VertexBuffer* getCpuSkinnedVertexBuffer() const { return m_skinnedVertexBuffer.get(); }

    bool isCpuSkinningFallbackEnabled() const { return m_enableCpuSkinningFallback; }
    void setCpuSkinningFallbackEnabled(bool enabled) { m_enableCpuSkinningFallback = enabled; }

    const VertexBuffer* getCurrentGpuSkinnedVertexBuffer() const;
    ID3D12Resource* getCurrentGpuSkinnedOutputResource() const;
    ID3D12Resource* getCurrentGpuPaletteModelResource() const;
    ID3D12Resource* getCurrentGpuPaletteNormalResource() const;

    bool hasGpuSkinningResources() const;

    void drawUi();

private:
    bool ensureSkinLoaded();
    bool resolveSkinBindings(GameObject* owner);
    void invalidateSkinningRuntime();

    void rebuildMatrixPalette();

    bool ensureSourceVerticesCached(MeshRenderer& renderer);
    void cacheSourceVertices(const MeshAsset& meshAsset);
    void rebuildCpuSkinnedVertexBuffer();

    bool ensureGpuSkinningResources();
    void updateGpuPaletteBuffers(MeshRenderer& renderer);
    void invalidateGpuSkinningResources();

private:
    AssetReference m_skinAsset{};

    std::shared_ptr<SkinAsset> m_skin;
    std::vector<Transform*> m_jointTransforms;
    bool m_skinBindingsResolved = false;

    std::vector<Matrix> m_matrixPalette;
    std::vector<Matrix> m_normalPalette;

    std::vector<Vertex> m_sourceVertices;
    std::vector<Vertex> m_skinnedVertices;
    std::unique_ptr<VertexBuffer> m_skinnedVertexBuffer;

    bool m_enableCpuSkinningFallback = false;
    AssetReference m_cachedMeshAsset{};

    std::unique_ptr<VertexBuffer> m_gpuSkinnedVertexBuffers[FRAMES_IN_FLIGHT];

    ComPtr<ID3D12Resource> m_gpuPaletteModelBuffers[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12Resource> m_gpuPaletteNormalBuffers[FRAMES_IN_FLIGHT];

    size_t m_gpuSkinningVertexCapacity = 0;
    size_t m_gpuPaletteJointCapacity = 0;
};