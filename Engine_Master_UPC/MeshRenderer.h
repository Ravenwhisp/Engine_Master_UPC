#pragma once
#include "Component.h"
#include "BasicMesh.h"
#include "BasicMaterial.h"
#include "BoundingBox.h"
#include "MeshAsset.h"
#include "MaterialAsset.h"

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

	void addMesh(MeshAsset& model);
	void addMaterial(MaterialAsset& material);

	BasicMesh* getMesh() const { return m_mesh.get(); }
	BasicMaterial* getMaterial() const { return m_material.get(); }
	bool									hasMesh() const { return m_mesh != nullptr; }

	Engine::BoundingBox& getBoundingBox() { return m_boundingBox; }

	void drawUi() override;

	void onTransformChange() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	int getTriangles() const { return m_triangles; }

	MD5Hash& getMeshReference() { return m_meshAsset; }
	MD5Hash& getMaterialReference() { return m_materialAsset; }

private:
	std::shared_ptr<BasicMesh>		m_mesh;
	std::shared_ptr<BasicMaterial>	m_material;

	MD5Hash							m_meshAsset = INVALID_ASSET_ID;
	MD5Hash							m_materialAsset = INVALID_ASSET_ID;

	Engine::BoundingBox				m_boundingBox;

	int m_triangles = 0;
};