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

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

	int getTriangles() const { return m_triangles; }

	MD5Hash& getMeshReference() { return m_meshAsset; }
	std::vector<MD5Hash>& getMaterialsReference() { return m_materialAssets; }

	IDebugDrawable* getAsDebugDrawable() { return static_cast<IDebugDrawable*>(this); }
 
	// Legacy only: used to migrate old prefabs/scenes that stored SkinAssetId inside MeshRenderer.
	const MD5Hash& getSkinReference() const { return m_skinAsset; }
	void setLegacySkinReference(const MD5Hash& skinUID) { m_skinAsset = skinUID; }

	bool hasSkin() const { return m_skin != nullptr; }

	Skin* getSkin() { return m_skin.get(); }
	const Skin* getSkin() const { return m_skin.get(); }

	Skin& ensureSkin();
	void clearSkin();

	const bool isCulled() { return m_isCulled; }
	void setIsCulled(bool culled) { m_isCulled = culled; }

private:
	std::shared_ptr<BasicMesh>		m_mesh;
	std::unique_ptr<Skin>			m_skin;
	// The position of the material corresponds to the submesh number
	std::vector<std::shared_ptr<BasicMaterial>>	m_materials;

	MD5Hash							m_meshAsset = INVALID_ASSET_ID;
	MD5Hash							m_skinAsset = INVALID_ASSET_ID; // Legacy only while migrating old prefabs/scenes that stored SkinAssetId inside MeshRenderer.
	std::vector<MD5Hash>			m_materialAssets;

	mutable Engine::BoundingBox				m_boundingBox;

	int m_triangles = 0;

	bool m_isCulled = false;
};