#pragma once
#include "Component.h"
#include "BasicMaterial.h"
#include "MeshAsset.h"
#include "BoundingBox.h"
#include "IDebugDrawable.h"

class BasicMesh;
class MaterialAsset;

namespace tinygltf { class Model; }

struct ModelData
{
	Matrix model;
	Matrix normalMat;
	BasicMaterial::BDRFPhongMaterialData material;
};


class MeshRenderer : public Component, public IDebugDrawable
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

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	int getTriangles() const { return m_triangles; }

	MD5Hash& getMeshReference() { return m_meshAsset; }
	std::vector<MD5Hash>& getMaterialsReference() { return m_materialAssets; }

	MD5Hash& getSkinReference() { return m_skinAsset; }
	const MD5Hash& getSkinReference() const { return m_skinAsset; }

private:
	std::shared_ptr<BasicMesh>		m_mesh;
	// The position of the material corresponds to the submesh number
	std::vector<std::shared_ptr<BasicMaterial>>	m_materials;

	MD5Hash							m_meshAsset = INVALID_ASSET_ID;
	MD5Hash							m_skinAsset = INVALID_ASSET_ID;
	std::vector<MD5Hash>			m_materialAssets;

	mutable Engine::BoundingBox				m_boundingBox;

	int m_triangles = 0;
};