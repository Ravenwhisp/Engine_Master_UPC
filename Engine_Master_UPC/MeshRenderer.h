#pragma once
#include "Component.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"
#include "BoundingBox.h"
#include "ModelAsset.h"

namespace tinygltf { class Model; }

struct ModelData 
{
	Matrix model;
	Matrix normalMat;
	BasicMaterial::BDRFPhongMaterialData material;
};


class MeshRenderer : public Component
{
public:
	MeshRenderer(UID id, GameObject* gameObject) : Component(id, ComponentType::MODEL, gameObject) {};
	~MeshRenderer() = default;

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void addModel(ModelAsset& model);

	std::vector<std::shared_ptr<BasicMesh>>&		getMeshes() const { return m_meshes; }
	std::vector<std::shared_ptr<BasicMaterial>>&	getMaterials() const { return m_materials; }
	bool											hasMeshes() { return m_meshes.size() != 0; }
	MD5Hash												getModelAssetId() const { return m_modelAssetId; }

	BasicMaterial* getMaterial(const MD5Hash& materialId)
	{
		auto it = m_materialIndexByUID.find(materialId);
		return m_materials[it->second].get();
	}

	Engine::BoundingBox&							getBoundingBox() { return m_boundingBox; }

	void drawUi() override;

	void onTransformChange() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	int getTriangles() { return m_triangles; }

private:
	mutable std::vector<std::shared_ptr<BasicMesh>>		m_meshes;
	mutable std::vector<std::shared_ptr<BasicMaterial>>	m_materials;
	std::unordered_map<MD5Hash, uint32_t>					m_materialIndexByUID;

	Engine::BoundingBox									m_boundingBox;

	MD5Hash m_modelAssetId = INVALID_ASSET_ID;

	std::string m_modelPath;
	std::string m_basePath;

	int m_triangles;
};
