#pragma once
#include "Component.h"
#include "BasicMaterial.h"
#include "MeshAsset.h"
#include "BoundingBox.h"
#include "IDebugDrawable.h"
#include "ShadowCasterHandle.h"

#include "BasicMesh.h"
#include "Skin.h"

#include <memory>
#include <cstdint>

class MaterialAsset;

namespace tinygltf
{
    class Model;
}

struct ModelData
{
    Matrix model;
    Matrix normalMat;
    BasicMaterial::PbrMetallicRoughnessData material;
};

enum class RenderMode : UINT
{
    DEFAULT = 0,
    TRANSP = 1,
    FLOW_MAP = 2,
    COUNT = 3
};

class MeshRenderer : public Component
{
public:
    MeshRenderer(UID id, GameObject* gameObject)
        : Component(id, ComponentType::MODEL, gameObject)
    {
    }

    ~MeshRenderer() override;

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    void addMesh(MeshAsset& model, bool recalculateBounds = true);
    void addMaterial(MaterialAsset& material);

    std::shared_ptr<BasicMesh>& getMesh() { return m_mesh; }
    std::vector<std::shared_ptr<BasicMaterial>>& getMaterials() { return m_materials; }

    bool hasMesh() const { return m_mesh != nullptr; }

    Engine::BoundingBox& getBoundingBox() { return m_boundingBox; }
    const Engine::BoundingBox& getBoundingBox() const { return m_boundingBox; }

    bool init() override;
    bool cleanUp() override;
    void drawUi() override;
    void debugDraw() override;
    void onTransformChange() override;
    void onTransformDirty() override;
    void onActiveChange() override;
    void onHierarchyActiveChange() override;
    void update() override;

    void registerShadowCaster();
    void unregisterShadowCaster();

    void serialize(IArchive& archive) override;
    void fixReferences(const SceneReferenceResolver& resolver) override;

    int getTriangles() const { return m_triangles; }

    void setMeshReference(AssetId& meshRef);
    AssetId& getMeshReference() { return m_meshAsset; }

    void addMaterialReference(AssetId& materialRef);
    std::vector<AssetId>& getMaterialsReference() { return m_materialAssets; }

    IDebugDrawable* getAsDebugDrawable() { return static_cast<IDebugDrawable*>(this); }

    AssetId& getSkinReference() { return m_skinAsset; }

    void setSkinReference(AssetId& skinUID);

    bool hasSkin() const { return m_skin != nullptr; }
    bool hasSkinningConfiguration() const { return m_skinAsset.isValid(); }

    Skin* getSkin() { return m_skin.get(); }
    const Skin* getSkin() const { return m_skin.get(); }

    Skin& ensureSkin();
    void clearSkin();

    bool isCulled() const { return m_isCulled; }
    void setIsCulled(bool culled) { m_isCulled = culled; }

    RenderMode getRenderMode() const { return m_renderMode; }

    bool getCastShadows() const { return m_castShadows; }
    void setCastShadows(bool castShadows);
    const ShadowCasterHandle& getShadowCasterHandle() const { return m_shadowCasterHandle; }
    void setShadowCasterHandle(const ShadowCasterHandle& handle) { m_shadowCasterHandle = handle; }
    void clearShadowCasterHandle() { m_shadowCasterHandle.reset(); }

    uint64_t getShadowCandidateRevision() const { return m_shadowCandidateRevision; }

private:
    void recompute();
    void recalculateBoundingBox();
    void updateBoundingBoxWorld();
    void markShadowCandidateDirty();

    std::shared_ptr<BasicMesh> m_mesh;
    std::unique_ptr<Skin> m_skin;

    std::vector<std::shared_ptr<BasicMaterial>> m_materials;

    AssetId m_meshAsset{};
    AssetId m_skinAsset{};
    std::vector<AssetId> m_materialAssets{};

    Engine::BoundingBox m_boundingBox;

    bool m_customBoundingBox = false;

    int m_triangles = 0;

    bool m_isCulled = false;

    bool m_castShadows = true;

    uint64_t m_shadowCandidateRevision = 1;

    ShadowCasterHandle m_shadowCasterHandle{};

    RenderMode m_renderMode = RenderMode::DEFAULT;
};