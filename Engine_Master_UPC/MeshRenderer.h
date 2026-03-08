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
	BasicMaterial::PbrMetallicRoughnessData material;
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

	BasicMaterial* getMaterial(UID materialId)
	{
		auto it = m_materialIndexByUID.find(materialId);
		return m_materials[it->second].get();
	}

	bool											getHasBounds() { return m_hasBounds; }
	Engine::BoundingBox&							getBoundingBox() { return m_boundingBox; }

#pragma region Loop functions
	bool init() override;
	void render(ID3D12GraphicsCommandList* commandList, Matrix& viewMatrix, Matrix& projectionMatrix) override;
	bool cleanUp() override;
#pragma endregion

	void drawUi() override;

	void onTransformChange() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
	mutable std::vector<std::shared_ptr<BasicMesh>>		m_meshes;
	mutable std::vector<std::shared_ptr<BasicMaterial>>	m_materials;
	std::unordered_map<UID, uint32_t>					m_materialIndexByUID;

	Engine::BoundingBox									m_boundingBox;

	UID m_modelAssetId = 0;

	std::string m_modelPath;
	std::string m_basePath;

	bool m_boundsDepthTest = true;
	bool m_drawWorldAabb = false;

	bool    m_hasBounds = false;
};
