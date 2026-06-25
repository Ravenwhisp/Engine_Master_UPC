#pragma once
#include "Component.h"
#include "BasicMaterial.h"
#include "MeshAsset.h"
#include "BoundingBox.h"
#include "IDebugDrawable.h"

#include "BasicMesh.h"
#include "Skin.h"

#include <memory>

class MaterialAsset;

namespace tinygltf { class Model; }

struct ModelData
{
	Matrix model;
	Matrix normalMat;
	BasicMaterial::PbrMetallicRoughnessData material;
};


class MeshRenderer : public Component
{
public:
    enum ShaderType : uint32_t
    {
        SHADER_STANDARD_PBR = 0x1,
        SHADER_RIM_EROSION  = 0x2,
    };

    MeshRenderer(UID id, GameObject* gameObject) : Component(id, ComponentType::MODEL, gameObject) {};
    ~MeshRenderer() override;

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void addMesh(MeshAsset& model);
	void addMaterial(MaterialAsset& material);

	std::shared_ptr<BasicMesh>& getMesh() { return m_mesh; }
	std::vector<std::shared_ptr<BasicMaterial>>& getMaterials() { return m_materials; }

	bool									hasMesh() const { return m_mesh != nullptr; }

	Engine::BoundingBox& getBoundingBox() const { return m_boundingBox; }

	void drawUi() override;
	void debugDraw() override;
	void onTransformChange() override;
	void update() override;

	void serialize(IArchive& archive) override;
	void fixReferences(const SceneReferenceResolver& resolver) override;

	int getTriangles() const { return m_triangles; }

	void setMeshReference(AssetReference& meshRef);
	AssetReference& getMeshReference() { return m_meshAsset; }
	void addMaterialReference(AssetReference& materialRef);
	std::vector<AssetReference>& getMaterialsReference() { return m_materialAssets; }

	IDebugDrawable* getAsDebugDrawable() { return static_cast<IDebugDrawable*>(this); }
 
	// Legacy only: used to migrate old prefabs/scenes that stored SkinAssetId inside MeshRenderer.
	AssetReference& getSkinReference() { return m_skinAsset; }
	void setSkinReference(AssetReference& skinUID) { m_skinAsset = skinUID; }

	bool hasSkin() const { return m_skin != nullptr; }

	Skin* getSkin() { return m_skin.get(); }
	const Skin* getSkin() const { return m_skin.get(); }

	Skin& ensureSkin();
	void clearSkin();

    const bool isCulled() { return m_isCulled; }
    void setIsCulled(bool culled) { m_isCulled = culled; }

    uint32_t getShaderFlags() const { return m_shaderFlags; }
    void setShaderFlags(uint32_t flags) { m_shaderFlags = flags; }
    bool hasShaderFlag(uint32_t flag) const { return (m_shaderFlags & flag) != 0; }

private:
	void recompute();

	std::shared_ptr<BasicMesh>		m_mesh;
	std::unique_ptr<Skin>			m_skin;
	// The position of the material corresponds to the submesh number
	std::vector<std::shared_ptr<BasicMaterial>>	m_materials;

	AssetReference m_meshAsset{};
	AssetReference m_skinAsset{};
	std::vector<AssetReference> m_materialAssets{};

	mutable Engine::BoundingBox				m_boundingBox;

    int m_triangles = 0;

    bool m_isCulled = false;

    uint32_t m_shaderFlags = ShaderType::SHADER_STANDARD_PBR;
};