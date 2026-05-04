#pragma once

#include "Component.h"
#include "MD5Fwd.h"
#include "Vertex.h"
#include "VertexBuffer.h"

#include "SimpleMath.h"

#include <memory>
#include <vector>

class SkinAsset;
class Transform;
class MeshAsset;

class SkinComponent final : public Component
{
public:
    SkinComponent(UID id, GameObject* owner);
    ~SkinComponent() override = default;

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool init() override;
    void lateUpdate() override;
    bool cleanUp() override;

    void setSkinReference(const MD5Hash& skinUID);
    const MD5Hash& getSkinReference() const { return m_skinAsset; }

    const std::shared_ptr<SkinAsset>& getSkin() const { return m_skin; }
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

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

private:
    bool ensureSkinLoaded();
    bool resolveSkinBindings();
    void invalidateSkinningRuntime();
    void rebuildMatrixPalette();
    bool ensureSourceVerticesCached();
    void cacheSourceVertices(const MeshAsset& meshAsset);
    void rebuildCpuSkinnedVertexBuffer();
    bool ensureGpuSkinningResources();
    void updateGpuPaletteBuffers();
    void invalidateGpuSkinningResources();

private:
    MD5Hash m_skinAsset = INVALID_ASSET_ID;

    std::shared_ptr<SkinAsset> m_skin;
    std::vector<Transform*> m_jointTransforms;
    bool m_skinBindingsResolved = false;

    std::vector<Matrix> m_matrixPalette;
    std::vector<Matrix> m_normalPalette;

    std::vector<Vertex> m_sourceVertices;
    std::vector<Vertex> m_skinnedVertices;
    std::unique_ptr<VertexBuffer> m_skinnedVertexBuffer;

    bool m_enableCpuSkinningFallback = false;
    MD5Hash m_cachedMeshAsset = INVALID_ASSET_ID;

    std::unique_ptr<VertexBuffer> m_gpuSkinnedVertexBuffers[FRAMES_IN_FLIGHT];

    ComPtr<ID3D12Resource> m_gpuPaletteModelBuffers[FRAMES_IN_FLIGHT];
    ComPtr<ID3D12Resource> m_gpuPaletteNormalBuffers[FRAMES_IN_FLIGHT];

    size_t m_gpuSkinningVertexCapacity = 0;
    size_t m_gpuPaletteJointCapacity = 0;
};