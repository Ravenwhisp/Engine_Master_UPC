#pragma once
#include "Component.h"
#include "BasicMaterial.h"
#include "MeshAsset.h"
#include "BoundingBox.h"
#include "IDebugDrawable.h"

#include "BasicMesh.h"
#include "Vertex.h"
#include "VertexBuffer.h"
#include <AssetReference.h>

class BasicMesh;
class MaterialAsset;
class SkinAsset;
class Transform;

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
	MeshRenderer(UID id, GameObject* gameObject) : Component(id, ComponentType::MODEL, gameObject) {};
	~MeshRenderer() = default;

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

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	int getTriangles() const { return m_triangles; }

	AssetReference& getMeshReference() { return m_meshAsset; }
	std::vector<AssetReference>& getMaterialsReference() { return m_materialAssets; }

	IDebugDrawable* getAsDebugDrawable() { return static_cast<IDebugDrawable*>(this); }

	AssetReference& getSkinReference() { return m_skinAsset; }
	const AssetReference& getSkinReference() const { return m_skinAsset; }

	const std::vector<Matrix>& getMatrixPalette() const { return m_matrixPalette; }
	const std::vector<Matrix>& getNormalPalette() const { return m_normalPalette; }
	bool hasSkinPalette() const { return !m_matrixPalette.empty(); }

	const VertexBuffer* getCurrentGpuSkinnedVertexBuffer() const;
	ID3D12Resource* getCurrentGpuSkinnedOutputResource() const;
	ID3D12Resource* getCurrentGpuPaletteModelResource() const;
	ID3D12Resource* getCurrentGpuPaletteNormalResource() const;

	uint32_t getSkinningVertexCount() const { return static_cast<uint32_t>(m_sourceVertices.size()); }
	bool hasGpuSkinningResources() const;

	const VertexBuffer* getCpuSkinnedVertexBuffer() const { return m_skinnedVertexBuffer.get(); }
	bool isCpuSkinningFallbackEnabled() const { return m_enableCpuSkinningFallback; }

	const bool isCulled() { return m_isCulled; }
	void setIsCulled(bool culled) { m_isCulled = culled; }

private:
	bool ensureSkinLoaded();
	bool resolveSkinBindings();
	void rebuildMatrixPalette();
	void invalidateSkinningRuntime();

	bool ensureGpuSkinningResources();
	void updateGpuPaletteBuffers();
	void invalidateGpuSkinningResources();

	void cacheSourceVertices(const MeshAsset& meshAsset);
	void rebuildCpuSkinnedVertexBuffer();

private:
	std::shared_ptr<SkinAsset>  m_skin;
	std::vector<Transform*>     m_jointTransforms;
	std::vector<Matrix>         m_matrixPalette;
	std::vector<Matrix>         m_normalPalette;
	bool                        m_skinBindingsResolved = false;
	bool						m_enableCpuSkinningFallback = false;
	std::shared_ptr<BasicMesh>		m_mesh;
	// The position of the material corresponds to the submesh number
	std::vector<std::shared_ptr<BasicMaterial>>	m_materials;

	std::vector<Vertex>                m_sourceVertices;
	std::vector<Vertex>                m_skinnedVertices;
	std::unique_ptr<VertexBuffer>      m_skinnedVertexBuffer;

	AssetReference							m_meshAsset = {};
	AssetReference							m_skinAsset = {};
	std::vector<AssetReference>				m_materialAssets;

	mutable Engine::BoundingBox				m_boundingBox;

	int m_triangles = 0;

	std::unique_ptr<VertexBuffer>  m_gpuSkinnedVertexBuffers[FRAMES_IN_FLIGHT];
	ComPtr<ID3D12Resource>         m_gpuPaletteModelBuffers[FRAMES_IN_FLIGHT];
	ComPtr<ID3D12Resource>         m_gpuPaletteNormalBuffers[FRAMES_IN_FLIGHT];

	size_t                         m_gpuSkinningVertexCapacity = 0;
	size_t                         m_gpuPaletteJointCapacity = 0;

	bool m_isCulled = false;

#pragma region Serialization
	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Component>(this), m_meshAsset, m_skinAsset, m_materialAssets);
	}
#pragma endregion
};

CEREAL_REGISTER_TYPE(MeshRenderer)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component, MeshRenderer)

